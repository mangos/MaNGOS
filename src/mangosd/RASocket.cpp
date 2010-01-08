/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "RASocket.h"
#include "World.h"
#include "Config/ConfigEnv.h"
#include "Util.h"
#include "AccountMgr.h"
#include "Language.h"
#include "ObjectMgr.h"

/// \todo Make this thread safe if in the future 2 admins should be able to log at the same time.
SOCKET r;

uint32 iSession=0;                                          ///< Session number (incremented each time a new connection is made)
unsigned int iUsers=0;                                      ///< Number of active administrators

typedef int(* pPrintf)(const char*,...);

void ParseCommand(CliCommandHolder::Print*, char*command);

/// RASocket constructor
RASocket::RASocket(ISocketHandler &h): TcpSocket(h)
{

    ///- Increment the session number
    iSess =iSession++ ;

    ///- Get the config parameters
    bSecure = sConfig.GetBoolDefault( "RA.Secure", true );
    iMinLevel = sConfig.GetIntDefault( "RA.MinLevel", 3 );

    ///- Initialize buffer and data
    iInputLength=0;
    buff=new char[RA_BUFF_SIZE];
    stage=NONE;
}

/// RASocket destructor
RASocket::~RASocket()
{
    ///- Delete buffer and decrease active admins count
    delete [] buff;

    sLog.outRALog("Connection was closed.\n");

    if(stage==OK)
        iUsers--;
}

/// Accept an incoming connection
void RASocket::OnAccept()
{
    std::string ss=GetRemoteAddress();
    sLog.outRALog("Incoming connection from %s.\n",ss.c_str());
    ///- If there is already an active admin, drop the connection
    if(iUsers)
    {
        Sendf(sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_BUSY));
        SetCloseAndDelete();
        return;
    }

    ///- Else print Motd
    Sendf("%s\r\n",sWorld.GetMotd());
    Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_USER));
}

/// Read data from the network
void RASocket::OnRead()
{
    ///- Read data and check input length
    TcpSocket::OnRead();

    unsigned int sz=ibuf.GetLength();
    if (iInputLength+sz>=RA_BUFF_SIZE)
    {
        sLog.outRALog("Input buffer overflow, possible DOS attack.\n");
        SetCloseAndDelete();
        return;
    }

    ///- If there is already an active admin (other than you), drop the connection
    if (stage!=OK && iUsers)
    {
        Sendf(sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_BUSY));
        SetCloseAndDelete();
        return;
    }

    char *inp = new char [sz+1];
    ibuf.Read(inp,sz);

    ///- Discard data after line break or line feed
    bool gotenter=false;
    unsigned int y=0;
    for(;y<sz;y++)
    {
        if (inp[y]=='\r'||inp[y]=='\n')
        {
            gotenter=true;
            break;
        }
    }

    //No buffer overflow (checked above)
    memcpy(&buff[iInputLength],inp,y);
    iInputLength+=y;
    delete [] inp;
    if (gotenter)
    {
        buff[iInputLength]=0;
        iInputLength=0;
        switch(stage)
        {
            /// <ul> <li> If the input is 'USER <username>'
            case NONE:
            {
                ///- If we're interactive we don't expect "USER " to be there
                szLogin=&buff[0];

                ///- Get the gmlevel from the account table
                std::string login = szLogin;

                ///- Convert Account name to Upper Format
                AccountMgr::normalizeString(login);

                ///- Escape the Login to allow quotes in names
                loginDatabase.escape_string(login);

                QueryResult* result = loginDatabase.PQuery("SELECT gmlevel FROM account WHERE username = '%s'",login.c_str());

                ///- If the user is not found, deny access
                if(!result)
                {
                    Sendf("-No such user.\r\n");
                    sLog.outRALog("User %s does not exist.\n",szLogin.c_str());
                    if(bSecure)SetCloseAndDelete();
                    Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_USER));
                }
                else
                {
                    Field *fields = result->Fetch();

                    ///- if gmlevel is too low, deny access
                    if (fields[0].GetUInt32()<iMinLevel)
                    {
                        Sendf("-Not enough privileges.\r\n");
                        sLog.outRALog("User %s has no privilege.\n",szLogin.c_str());
                        if(bSecure)SetCloseAndDelete();
                        Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_USER));
                    }
                    else
                    {
                        stage=LG;
                        Sendf(sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_PASS));
                    }
                    delete result;
                }
                break;
            }
            ///<li> If the input is 'PASS <password>' (and the user already gave his username)
            case LG:
            {                                               //login+pass ok
                ///- If password is correct, increment the number of active administrators
                std::string login = szLogin;

                ///- If we're interactive we don't expect "PASS " to be there
                std::string pw = &buff[0];

                AccountMgr::normalizeString(login);
                AccountMgr::normalizeString(pw);
                loginDatabase.escape_string(login);
                loginDatabase.escape_string(pw);

                QueryResult *check = loginDatabase.PQuery(
                    "SELECT 1 FROM account WHERE username = '%s' AND sha_pass_hash=SHA1(CONCAT(username,':','%s'))",
                    login.c_str(), pw.c_str());

                if (check)
                {
                    delete check;
                    r=GetSocket();
                    stage=OK;
                    ++iUsers;

                    Sendf("+Logged in.\r\n");
                    sLog.outRALog("User %s has logged in.\n",szLogin.c_str());
                    Sendf("mangos>");
                }
                else
                {
                    ///- Else deny access
                    Sendf("-Wrong pass.\r\n");
                    sLog.outRALog("User %s has failed to log in.\n",szLogin.c_str());
                    if(bSecure)SetCloseAndDelete();
                    Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_PASS));
                }
                break;
            }
            ///<li> If user is logged, parse and execute the command
            case OK:
                if (strlen(buff))
                {
                    sLog.outRALog("Got '%s' cmd.\n",buff);
                    if (strncmp(buff,"quit",4)==0)
                        SetCloseAndDelete();
                    else
                        sWorld.QueueCliCommand(&RASocket::zprint, buff);
                }
                else
                    Sendf("mangos>");
                break;
            ///</ul>
        };

    }
}

/// Output function
void RASocket::zprint( const char * szText )
{
    if( !szText )
        return;

    #ifdef RA_CRYPT

    char *megabuffer = mangos_strdup(szText);
    unsigned int sz=strlen(megabuffer);
    Encrypt(megabuffer,sz);
    send(r,megabuffer,sz,0);
    delete [] megabuffer;

    #else

    unsigned int sz=strlen(szText);
    send(r,szText,sz,0);

    #endif
}
