/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** \file
    \ingroup mangosd
*/

#include "WorldSocketMgr.h"
#include "Common.h"
#include "Master.h"
#include "WorldSocket.h"
#include "WorldRunnable.h"
#include "World.h"
#include "Log.h"
#include "Timer.h"
#include "Policies/SingletonImp.h"
#include "SystemConfig.h"
#include "Config/Config.h"
#include "Database/DatabaseEnv.h"
#include "CliRunnable.h"
#include "RASocket.h"
#include "Util.h"
#include "revision_sql.h"
#include "MaNGOSsoap.h"
#include "MassMailMgr.h"
#include "DBCStores.h"

#include <ace/OS_NS_signal.h>
#include <ace/TP_Reactor.h>
#include <ace/Dev_Poll_Reactor.h>

#ifdef WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#endif

INSTANTIATE_SINGLETON_1( Master );

volatile uint32 Master::m_masterLoopCounter = 0;

class FreezeDetectorRunnable : public ACE_Based::Runnable
{
public:
    FreezeDetectorRunnable() { _delaytime = 0; }
    uint32 m_loops, m_lastchange;
    uint32 w_loops, w_lastchange;
    uint32 _delaytime;
    void SetDelayTime(uint32 t) { _delaytime = t; }
    void run(void)
    {
        if(!_delaytime)
            return;
        sLog.outString("Starting up anti-freeze thread (%u seconds max stuck time)...",_delaytime/1000);
        m_loops = 0;
        w_loops = 0;
        m_lastchange = 0;
        w_lastchange = 0;
        while(!World::IsStopped())
        {
            ACE_Based::Thread::Sleep(1000);

            uint32 curtime = WorldTimer::getMSTime();
            //DEBUG_LOG("anti-freeze: time=%u, counters=[%u; %u]",curtime,Master::m_masterLoopCounter,World::m_worldLoopCounter);

            // normal work
            if (w_loops != World::m_worldLoopCounter)
            {
                w_lastchange = curtime;
                w_loops = World::m_worldLoopCounter;
            }
            // possible freeze
            else if (WorldTimer::getMSTimeDiff(w_lastchange, curtime) > _delaytime)
            {
                sLog.outError("World Thread hangs, kicking out server!");
                *((uint32 volatile*)NULL) = 0;              // bang crash
            }
        }
        sLog.outString("Anti-freeze thread exiting without problems.");
    }
};

class RARunnable : public ACE_Based::Runnable
{
private:
    ACE_Reactor *m_Reactor;
    RASocket::Acceptor *m_Acceptor;
public:
    RARunnable()
    {
        ACE_Reactor_Impl* imp = 0;

        #if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)

        imp = new ACE_Dev_Poll_Reactor ();

        imp->max_notify_iterations (128);
        imp->restart (1);

        #else

        imp = new ACE_TP_Reactor ();
        imp->max_notify_iterations (128);

        #endif

        m_Reactor = new ACE_Reactor (imp, 1 /* 1= delete implementation so we don't have to care */);

        m_Acceptor = new RASocket::Acceptor;

    }

    ~RARunnable()
    {
        delete m_Reactor;
        delete m_Acceptor;
    }

    void run ()
    {
        uint16 raport = sConfig.GetIntDefault ("Ra.Port", 3443);
        std::string stringip = sConfig.GetStringDefault ("Ra.IP", "0.0.0.0");

        ACE_INET_Addr listen_addr(raport, stringip.c_str());

        if (m_Acceptor->open (listen_addr, m_Reactor, ACE_NONBLOCK) == -1)
        {
            sLog.outError ("MaNGOS RA can not bind to port %d on %s", raport, stringip.c_str ());
        }

        sLog.outString ("Starting Remote access listner on port %d on %s", raport, stringip.c_str ());

        while (!m_Reactor->reactor_event_loop_done())
        {
            ACE_Time_Value interval (0, 10000);

            if (m_Reactor->run_reactor_event_loop (interval) == -1)
                break;

            if(World::IsStopped())
            {
                m_Acceptor->close();
                break;
            }
        }
        sLog.outString("RARunnable thread ended");
    }
};

Master::Master()
{
}

Master::~Master()
{
}

/// Main function
int Master::Run()
{
    /// worldd PID file creation
    std::string pidfile = sConfig.GetStringDefault("PidFile", "");
    if(!pidfile.empty())
    {
        uint32 pid = CreatePIDFile(pidfile);
        if( !pid )
        {
            sLog.outError( "Cannot create PID file %s.\n", pidfile.c_str() );
            Log::WaitBeforeContinueIfNeed();
            return 1;
        }

        sLog.outString( "Daemon PID: %u\n", pid );
    }

    ///- Start the databases
    if (!_StartDB())
    {
        Log::WaitBeforeContinueIfNeed();
        return 1;
    }

    ///- Initialize the World
    sWorld.SetInitialWorldSettings();

    //server loaded successfully => enable async DB requests
    //this is done to forbid any async transactions during server startup!
    CharacterDatabase.AllowAsyncTransactions();
    WorldDatabase.AllowAsyncTransactions();
    LoginDatabase.AllowAsyncTransactions();

    ///- Catch termination signals
    _HookSignals();

    ///- Launch WorldRunnable thread
    ACE_Based::Thread world_thread(new WorldRunnable);
    world_thread.setPriority(ACE_Based::Highest);

    // set realmbuilds depend on mangosd expected builds, and set server online
    {
        std::string builds = AcceptableClientBuildsListStr();
        LoginDatabase.escape_string(builds);
        LoginDatabase.DirectPExecute("UPDATE realmlist SET realmflags = realmflags & ~(%u), population = 0, realmbuilds = '%s'  WHERE id = '%u'", REALM_FLAG_OFFLINE, builds.c_str(), realmID);
    }

    ACE_Based::Thread* cliThread = NULL;

#ifdef WIN32
    if (sConfig.GetBoolDefault("Console.Enable", true) && (m_ServiceStatus == -1)/* need disable console in service mode*/)
#else
    if (sConfig.GetBoolDefault("Console.Enable", true))
#endif
    {
        ///- Launch CliRunnable thread
        cliThread = new ACE_Based::Thread(new CliRunnable);
    }

    ACE_Based::Thread* rar_thread = NULL;
    if(sConfig.GetBoolDefault ("Ra.Enable", false))
    {
        rar_thread = new ACE_Based::Thread(new RARunnable);
    }

    ///- Handle affinity for multiple processors and process priority on Windows
    #ifdef WIN32
    {
        HANDLE hProcess = GetCurrentProcess();

        uint32 Aff = sConfig.GetIntDefault("UseProcessors", 0);
        if(Aff > 0)
        {
            ULONG_PTR appAff;
            ULONG_PTR sysAff;

            if(GetProcessAffinityMask(hProcess,&appAff,&sysAff))
            {
                ULONG_PTR curAff = Aff & appAff;            // remove non accessible processors

                if(!curAff )
                {
                    sLog.outError("Processors marked in UseProcessors bitmask (hex) %x not accessible for mangosd. Accessible processors bitmask (hex): %x",Aff,appAff);
                }
                else
                {
                    if(SetProcessAffinityMask(hProcess,curAff))
                        sLog.outString("Using processors (bitmask, hex): %x", curAff);
                    else
                        sLog.outError("Can't set used processors (hex): %x",curAff);
                }
            }
            sLog.outString();
        }

        bool Prio = sConfig.GetBoolDefault("ProcessPriority", false);

//        if(Prio && (m_ServiceStatus == -1)/* need set to default process priority class in service mode*/)
        if(Prio)
        {
            if(SetPriorityClass(hProcess,HIGH_PRIORITY_CLASS))
                sLog.outString("mangosd process priority class set to HIGH");
            else
                sLog.outError("Can't set mangosd process priority class.");
            sLog.outString();
        }
    }
    #endif

    ///- Start soap serving thread
    ACE_Based::Thread* soap_thread = NULL;

    if(sConfig.GetBoolDefault("SOAP.Enabled", false))
    {
        MaNGOSsoapRunnable *runnable = new MaNGOSsoapRunnable();

        runnable->setListenArguments(sConfig.GetStringDefault("SOAP.IP", "127.0.0.1"), sConfig.GetIntDefault("SOAP.Port", 7878));
        soap_thread = new ACE_Based::Thread(runnable);
    }


    uint32 realCurrTime, realPrevTime;
    realCurrTime = realPrevTime = WorldTimer::getMSTime();

    ///- Start up freeze catcher thread
    ACE_Based::Thread* freeze_thread = NULL;
    if(uint32 freeze_delay = sConfig.GetIntDefault("MaxCoreStuckTime", 0))
    {
        FreezeDetectorRunnable *fdr = new FreezeDetectorRunnable();
        fdr->SetDelayTime(freeze_delay*1000);
        freeze_thread = new ACE_Based::Thread(fdr);
        freeze_thread->setPriority(ACE_Based::Highest);
    }

    ///- Launch the world listener socket
    uint16 wsport = sWorld.getConfig (CONFIG_UINT32_PORT_WORLD);
    std::string bind_ip = sConfig.GetStringDefault ("BindIP", "0.0.0.0");

    if (sWorldSocketMgr->StartNetwork (wsport, bind_ip) == -1)
    {
        sLog.outError ("Failed to start network");
        Log::WaitBeforeContinueIfNeed();
        World::StopNow(ERROR_EXIT_CODE);
        // go down and shutdown the server
    }

    sWorldSocketMgr->Wait ();

    ///- Stop freeze protection before shutdown tasks
    if (freeze_thread)
    {
        freeze_thread->destroy();
        delete freeze_thread;
    }

    ///- Stop soap thread
    if(soap_thread)
    {
        soap_thread->wait();
        soap_thread->destroy();
        delete soap_thread;
    }

    ///- Set server offline in realmlist
    LoginDatabase.DirectPExecute("UPDATE realmlist SET realmflags = realmflags | %u WHERE id = '%u'", REALM_FLAG_OFFLINE, realmID);

    ///- Remove signal handling before leaving
    _UnhookSignals();

    // when the main thread closes the singletons get unloaded
    // since worldrunnable uses them, it will crash if unloaded after master
    world_thread.wait();

    if(rar_thread)
    {
        rar_thread->wait();
        rar_thread->destroy();
        delete rar_thread;
    }

    ///- Clean account database before leaving
    clearOnlineAccounts();

    // send all still queued mass mails (before DB connections shutdown)
    sMassMailMgr.Update(true);

    ///- Wait for DB delay threads to end
    CharacterDatabase.HaltDelayThread();
    WorldDatabase.HaltDelayThread();
    LoginDatabase.HaltDelayThread();

    sLog.outString( "Halting process..." );

    if (cliThread)
    {
        #ifdef WIN32

        // this only way to terminate CLI thread exist at Win32 (alt. way exist only in Windows Vista API)
        //_exit(1);
        // send keyboard input to safely unblock the CLI thread
        INPUT_RECORD b[5];
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        b[0].EventType = KEY_EVENT;
        b[0].Event.KeyEvent.bKeyDown = TRUE;
        b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[0].Event.KeyEvent.wRepeatCount = 1;

        b[1].EventType = KEY_EVENT;
        b[1].Event.KeyEvent.bKeyDown = FALSE;
        b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[1].Event.KeyEvent.wRepeatCount = 1;

        b[2].EventType = KEY_EVENT;
        b[2].Event.KeyEvent.bKeyDown = TRUE;
        b[2].Event.KeyEvent.dwControlKeyState = 0;
        b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[2].Event.KeyEvent.wRepeatCount = 1;
        b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

        b[3].EventType = KEY_EVENT;
        b[3].Event.KeyEvent.bKeyDown = FALSE;
        b[3].Event.KeyEvent.dwControlKeyState = 0;
        b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
        b[3].Event.KeyEvent.wRepeatCount = 1;
        DWORD numb;
        BOOL ret = WriteConsoleInput(hStdIn, b, 4, &numb);

        cliThread->wait();

        #else

        cliThread->destroy();

        #endif

        delete cliThread;
    }

    ///- Exit the process with specified return value
    return World::GetExitCode();
}

/// Initialize connection to the databases
bool Master::_StartDB()
{
    ///- Get world database info from configuration file
    std::string dbstring = sConfig.GetStringDefault("WorldDatabaseInfo", "");
    int nConnections = sConfig.GetIntDefault("WorldDatabaseConnections", 1);
    if(dbstring.empty())
    {
        sLog.outError("Database not specified in configuration file");
        return false;
    }
    sLog.outString("World Database: %s, total connections: %i", dbstring.c_str(), nConnections + 1);

    ///- Initialise the world database
    if(!WorldDatabase.Initialize(dbstring.c_str(), nConnections))
    {
        sLog.outError("Cannot connect to world database %s",dbstring.c_str());
        return false;
    }

    if(!WorldDatabase.CheckRequiredField("db_version",REVISION_DB_MANGOS))
    {
        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        return false;
    }

    dbstring = sConfig.GetStringDefault("CharacterDatabaseInfo", "");
    nConnections = sConfig.GetIntDefault("CharacterDatabaseConnections", 1);
    if(dbstring.empty())
    {
        sLog.outError("Character Database not specified in configuration file");

        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        return false;
    }
    sLog.outString("Character Database: %s, total connections: %i", dbstring.c_str(), nConnections + 1);

    ///- Initialise the Character database
    if(!CharacterDatabase.Initialize(dbstring.c_str(), nConnections))
    {
        sLog.outError("Cannot connect to Character database %s",dbstring.c_str());

        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        return false;
    }

    if(!CharacterDatabase.CheckRequiredField("character_db_version",REVISION_DB_CHARACTERS))
    {
        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        return false;
    }

    ///- Get login database info from configuration file
    dbstring = sConfig.GetStringDefault("LoginDatabaseInfo", "");
    nConnections = sConfig.GetIntDefault("LoginDatabaseConnections", 1);
    if(dbstring.empty())
    {
        sLog.outError("Login database not specified in configuration file");

        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        return false;
    }

    ///- Initialise the login database
    sLog.outString("Login Database: %s, total connections: %i", dbstring.c_str(), nConnections + 1);
    if(!LoginDatabase.Initialize(dbstring.c_str(), nConnections))
    {
        sLog.outError("Cannot connect to login database %s",dbstring.c_str());

        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        return false;
    }

    if(!LoginDatabase.CheckRequiredField("realmd_db_version",REVISION_DB_REALMD))
    {
        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        LoginDatabase.HaltDelayThread();
        return false;
    }

    ///- Get the realm Id from the configuration file
    realmID = sConfig.GetIntDefault("RealmID", 0);
    if(!realmID)
    {
        sLog.outError("Realm ID not defined in configuration file");

        ///- Wait for already started DB delay threads to end
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        LoginDatabase.HaltDelayThread();
        return false;
    }

    sLog.outString("Realm running as realm ID %d", realmID);

    ///- Clean the database before starting
    clearOnlineAccounts();

    sWorld.LoadDBVersion();

    sLog.outString("Using World DB: %s", sWorld.GetDBVersion());
    sLog.outString("Using creature EventAI: %s", sWorld.GetCreatureEventAIVersion());
    return true;
}

/// Clear 'online' status for all accounts with characters in this realm
void Master::clearOnlineAccounts()
{
    // Cleanup online status for characters hosted at current realm
    /// \todo Only accounts with characters logged on *this* realm should have online status reset. Move the online column from 'account' to 'realmcharacters'?
    LoginDatabase.PExecute("UPDATE account SET active_realm_id = 0 WHERE active_realm_id = '%u'", realmID);

    CharacterDatabase.Execute("UPDATE characters SET online = 0 WHERE online<>0");

    // Battleground instance ids reset at server restart
    CharacterDatabase.Execute("UPDATE character_battleground_data SET instance_id = 0");
}

/// Handle termination signals
void Master::_OnSignal(int s)
{
    switch (s)
    {
        case SIGINT:
            World::StopNow(RESTART_EXIT_CODE);
            break;
        case SIGTERM:
        #ifdef _WIN32
        case SIGBREAK:
        #endif
            World::StopNow(SHUTDOWN_EXIT_CODE);
            break;
    }

    signal(s, _OnSignal);
}

/// Define hook '_OnSignal' for all termination signals
void Master::_HookSignals()
{
    signal(SIGINT, _OnSignal);
    signal(SIGTERM, _OnSignal);
    #ifdef _WIN32
    signal(SIGBREAK, _OnSignal);
    #endif
}

/// Unhook the signals before leaving
void Master::_UnhookSignals()
{
    signal(SIGINT, 0);
    signal(SIGTERM, 0);
    #ifdef _WIN32
    signal(SIGBREAK, 0);
    #endif
}
