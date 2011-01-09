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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "World.h"
#include "Player.h"
#include "Opcodes.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "SystemConfig.h"
#include "revision.h"
#include "revision_nr.h"
#include "Util.h"

bool ChatHandler::HandleHelpCommand(char* args)
{
    if(!*args)
    {
        ShowHelpForCommand(getCommandTable(), "help");
        ShowHelpForCommand(getCommandTable(), "");
    }
    else
    {
        if (!ShowHelpForCommand(getCommandTable(), args))
            SendSysMessage(LANG_NO_CMD);
    }

    return true;
}

bool ChatHandler::HandleCommandsCommand(char* /*args*/)
{
    ShowHelpForCommand(getCommandTable(), "");
    return true;
}

bool ChatHandler::HandleAccountCommand(char* args)
{
    // let show subcommands at unexpected data in args
    if (*args)
        return false;

    AccountTypes gmlevel = GetAccessLevel();
    PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmlevel));
    return true;
}

bool ChatHandler::HandleStartCommand(char* /*args*/)
{
    Player *chr = m_session->GetPlayer();

    if(chr->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    if(chr->isInCombat())
    {
        SendSysMessage(LANG_YOU_IN_COMBAT);
        SetSentErrorMessage(true);
        return false;
    }

    // cast spell Stuck
    chr->CastSpell(chr,7355,false);
    return true;
}

bool ChatHandler::HandleServerInfoCommand(char* /*args*/)
{
    uint32 activeClientsNum = sWorld.GetActiveSessionCount();
    uint32 queuedClientsNum = sWorld.GetQueuedSessionCount();
    uint32 maxActiveClientsNum = sWorld.GetMaxActiveSessionCount();
    uint32 maxQueuedClientsNum = sWorld.GetMaxQueuedSessionCount();
    std::string str = secsToTimeString(sWorld.GetUptime());

    char const* full;
    if(m_session)
        full = _FULLVERSION(REVISION_DATE,REVISION_TIME,REVISION_NR,"|cffffffff|Hurl:" REVISION_ID "|h" REVISION_ID "|h|r");
    else
        full = _FULLVERSION(REVISION_DATE,REVISION_TIME,REVISION_NR,REVISION_ID);
    SendSysMessage(full);

    if (sScriptMgr.IsScriptLibraryLoaded())
    {
        char const* ver = sScriptMgr.GetScriptLibraryVersion();
        if (ver && *ver)
            PSendSysMessage(LANG_USING_SCRIPT_LIB, ver);
        else
            SendSysMessage(LANG_USING_SCRIPT_LIB_UNKNOWN);
    }
    else
        SendSysMessage(LANG_USING_SCRIPT_LIB_NONE);

    PSendSysMessage(LANG_USING_WORLD_DB,sWorld.GetDBVersion());
    PSendSysMessage(LANG_USING_EVENT_AI,sWorld.GetCreatureEventAIVersion());
    PSendSysMessage(LANG_CONNECTED_USERS, activeClientsNum, maxActiveClientsNum, queuedClientsNum, maxQueuedClientsNum);
    PSendSysMessage(LANG_UPTIME, str.c_str());

    return true;
}

bool ChatHandler::HandleDismountCommand(char* /*args*/)
{
    //If player is not mounted, so go out :)
    if (!m_session->GetPlayer( )->IsMounted())
    {
        SendSysMessage(LANG_CHAR_NON_MOUNTED);
        SetSentErrorMessage(true);
        return false;
    }

    if(m_session->GetPlayer( )->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    m_session->GetPlayer()->Unmount();
    m_session->GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    return true;
}

bool ChatHandler::HandleSaveCommand(char* /*args*/)
{
    Player *player=m_session->GetPlayer();

    // save GM account without delay and output message (testing, etc)
    if(GetAccessLevel() > SEC_PLAYER)
    {
        player->SaveToDB();
        SendSysMessage(LANG_PLAYER_SAVED);
        return true;
    }

    // save or plan save after 20 sec (logout delay) if current next save time more this value and _not_ output any messages to prevent cheat planning
    uint32 save_interval = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);
    if (save_interval==0 || (save_interval > 20*IN_MILLISECONDS && player->GetSaveTimer() <= save_interval - 20*IN_MILLISECONDS))
        player->SaveToDB();

    return true;
}

bool ChatHandler::HandleGMListIngameCommand(char* /*args*/)
{
    std::list< std::pair<std::string, bool> > names;

    {
        HashMapHolder<Player>::ReadGuard g(HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType &m = sObjectAccessor.GetPlayers();
        for(HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            AccountTypes itr_sec = itr->second->GetSession()->GetSecurity();
            if ((itr->second->isGameMaster() || (itr_sec > SEC_PLAYER && itr_sec <= (AccountTypes)sWorld.getConfig(CONFIG_UINT32_GM_LEVEL_IN_GM_LIST))) &&
                (!m_session || itr->second->IsVisibleGloballyFor(m_session->GetPlayer())))
                names.push_back(std::make_pair<std::string, bool>(GetNameLink(itr->second), itr->second->isAcceptWhispers()));
        }
    }

    if (!names.empty())
    {
        SendSysMessage(LANG_GMS_ON_SRV);

        char const* accepts = GetMangosString(LANG_GM_ACCEPTS_WHISPER);
        char const* not_accept = GetMangosString(LANG_GM_NO_WHISPER);
        for(std::list<std::pair< std::string, bool> >::const_iterator iter = names.begin(); iter != names.end(); ++iter)
            PSendSysMessage("%s - %s", iter->first.c_str(), iter->second ? accepts : not_accept);
    }
    else
        SendSysMessage(LANG_GMS_NOT_LOGGED);

    return true;
}

bool ChatHandler::HandleAccountPasswordCommand(char* args)
{
    // allow use from RA, but not from console (not have associated account id)
    if (!GetAccountId())
    {
        SendSysMessage (LANG_RA_ONLY_COMMAND);
        SetSentErrorMessage (true);
        return false;
    }

    // allow or quoted string with possible spaces or literal without spaces
    char *old_pass = ExtractQuotedOrLiteralArg(&args);
    char *new_pass = ExtractQuotedOrLiteralArg(&args);
    char *new_pass_c = ExtractQuotedOrLiteralArg(&args);

    if (!old_pass || !new_pass || !new_pass_c)
        return false;

    std::string password_old = old_pass;
    std::string password_new = new_pass;
    std::string password_new_c = new_pass_c;

    if (password_new != password_new_c)
    {
        SendSysMessage (LANG_NEW_PASSWORDS_NOT_MATCH);
        SetSentErrorMessage (true);
        return false;
    }

    if (!sAccountMgr.CheckPassword (GetAccountId(), password_old))
    {
        SendSysMessage (LANG_COMMAND_WRONGOLDPASSWORD);
        SetSentErrorMessage (true);
        return false;
    }

    AccountOpResult result = sAccountMgr.ChangePassword(GetAccountId(), password_new);

    switch(result)
    {
        case AOR_OK:
            SendSysMessage(LANG_COMMAND_PASSWORD);
            break;
        case AOR_PASS_TOO_LONG:
            SendSysMessage(LANG_PASSWORD_TOO_LONG);
            SetSentErrorMessage(true);
            return false;
        case AOR_NAME_NOT_EXIST:                            // not possible case, don't want get account name for output
        default:
            SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
            SetSentErrorMessage(true);
            return false;
    }

    return true;
}

bool ChatHandler::HandleAccountLockCommand(char* args)
{
    // allow use from RA, but not from console (not have associated account id)
    if (!GetAccountId())
    {
        SendSysMessage (LANG_RA_ONLY_COMMAND);
        SetSentErrorMessage (true);
        return false;
    }

    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    if (value)
    {
        LoginDatabase.PExecute( "UPDATE account SET locked = '1' WHERE id = '%u'", GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
    }
    else
    {
        LoginDatabase.PExecute( "UPDATE account SET locked = '0' WHERE id = '%u'", GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
    }

    return true;
}

/// Display the 'Message of the day' for the realm
bool ChatHandler::HandleServerMotdCommand(char* /*args*/)
{
    PSendSysMessage(LANG_MOTD_CURRENT, sWorld.GetMotd());
    return true;
}
