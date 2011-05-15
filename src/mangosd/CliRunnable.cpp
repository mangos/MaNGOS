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

/// \addtogroup mangosd
/// @{
/// \file

#include "Common.h"
#include "Language.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "Config/Config.h"
#include "Util.h"
#include "AccountMgr.h"
#include "CliRunnable.h"
#include "MapManager.h"
#include "Player.h"
#include "Chat.h"

void utf8print(void* arg, const char* str)
{
#if PLATFORM == PLATFORM_WINDOWS
    wchar_t wtemp_buf[6000];
    size_t wtemp_len = 6000-1;
    if(!Utf8toWStr(str,strlen(str),wtemp_buf,wtemp_len))
        return;

    char temp_buf[6000];
    CharToOemBuffW(&wtemp_buf[0],&temp_buf[0],wtemp_len+1);
    printf("%s", temp_buf);
#else
    printf("%s", str);
#endif
}

void commandFinished(void*, bool sucess)
{
    printf("mangos>");
    fflush(stdout);
}

/// Delete a user account and all associated characters in this realm
/// \todo This function has to be enhanced to respect the login/realm split (delete char, delete account chars in realm, delete account chars in realm then delete account
bool ChatHandler::HandleAccountDeleteCommand(char* args)
{
    if (!*args)
        return false;

    std::string account_name;
    uint32 account_id = ExtractAccountId(&args, &account_name);
    if (!account_id)
        return false;

    /// Commands not recommended call from chat, but support anyway
    /// can delete only for account with less security
    /// This is also reject self apply in fact
    if (HasLowerSecurityAccount (NULL, account_id, true))
        return false;

    AccountOpResult result = sAccountMgr.DeleteAccount(account_id);
    switch(result)
    {
        case AOR_OK:
            PSendSysMessage(LANG_ACCOUNT_DELETED,account_name.c_str());
            break;
        case AOR_NAME_NOT_EXIST:
            PSendSysMessage(LANG_ACCOUNT_NOT_EXIST,account_name.c_str());
            SetSentErrorMessage(true);
            return false;
        case AOR_DB_INTERNAL_ERROR:
            PSendSysMessage(LANG_ACCOUNT_NOT_DELETED_SQL_ERROR,account_name.c_str());
            SetSentErrorMessage(true);
            return false;
        default:
            PSendSysMessage(LANG_ACCOUNT_NOT_DELETED,account_name.c_str());
            SetSentErrorMessage(true);
            return false;
    }

    return true;
}

/**
 * Collects all GUIDs (and related info) from deleted characters which are still in the database.
 *
 * @param foundList    a reference to an std::list which will be filled with info data
 * @param searchString the search string which either contains a player GUID (low part) or a part of the character-name
 * @return             returns false if there was a problem while selecting the characters (e.g. player name not normalizeable)
 */
bool ChatHandler::GetDeletedCharacterInfoList(DeletedInfoList& foundList, std::string searchString)
{
    QueryResult* resultChar;
    if (!searchString.empty())
    {
        // search by GUID
        if (isNumeric(searchString))
            resultChar = CharacterDatabase.PQuery("SELECT guid, deleteInfos_Name, deleteInfos_Account, deleteDate FROM characters WHERE deleteDate IS NOT NULL AND guid = %u", uint32(atoi(searchString.c_str())));
        // search by name
        else
        {
            if(!normalizePlayerName(searchString))
                return false;

            resultChar = CharacterDatabase.PQuery("SELECT guid, deleteInfos_Name, deleteInfos_Account, deleteDate FROM characters WHERE deleteDate IS NOT NULL AND deleteInfos_Name " _LIKE_ " " _CONCAT3_("'%%'", "'%s'", "'%%'"), searchString.c_str());
        }
    }
    else
        resultChar = CharacterDatabase.Query("SELECT guid, deleteInfos_Name, deleteInfos_Account, deleteDate FROM characters WHERE deleteDate IS NOT NULL");

    if (resultChar)
    {
        do
        {
            Field* fields = resultChar->Fetch();

            DeletedInfo info;

            info.lowguid    = fields[0].GetUInt32();
            info.name       = fields[1].GetCppString();
            info.accountId  = fields[2].GetUInt32();

            // account name will be empty for nonexistent account
            sAccountMgr.GetName (info.accountId, info.accountName);

            info.deleteDate = time_t(fields[3].GetUInt64());

            foundList.push_back(info);
        } while (resultChar->NextRow());

        delete resultChar;
    }

    return true;
}

/**
 * Generate WHERE guids list by deleted info in way preventing return too long where list for existed query string length limit.
 *
 * @param itr          a reference to an deleted info list iterator, it updated in function for possible next function call if list to long
 * @param itr_end      a reference to an deleted info list iterator end()
 * @return             returns generated where list string in form: 'guid IN (gui1, guid2, ...)'
 */
std::string ChatHandler::GenerateDeletedCharacterGUIDsWhereStr(DeletedInfoList::const_iterator& itr, DeletedInfoList::const_iterator const& itr_end)
{
    std::ostringstream wherestr;
    wherestr << "guid IN ('";
    for(; itr != itr_end; ++itr)
    {
        wherestr << itr->lowguid;

        if (wherestr.str().size() > MAX_QUERY_LEN - 50)     // near to max query
        {
            ++itr;
            break;
        }

        DeletedInfoList::const_iterator itr2 = itr;
        if(++itr2 != itr_end)
            wherestr << "','";
    }
    wherestr << "')";
    return wherestr.str();
}

/**
 * Shows all deleted characters which matches the given search string, expected non empty list
 *
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::DeletedInfoList
 *
 * @param foundList contains a list with all found deleted characters
 */
void ChatHandler::HandleCharacterDeletedListHelper(DeletedInfoList const& foundList)
{
    if (!m_session)
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_BAR);
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_HEADER);
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_BAR);
    }

    for (DeletedInfoList::const_iterator itr = foundList.begin(); itr != foundList.end(); ++itr)
    {
        std::string dateStr = TimeToTimestampStr(itr->deleteDate);

        if (!m_session)
            PSendSysMessage(LANG_CHARACTER_DELETED_LIST_LINE_CONSOLE,
                itr->lowguid, itr->name.c_str(), itr->accountName.empty() ? "<nonexistent>" : itr->accountName.c_str(),
                itr->accountId, dateStr.c_str());
        else
            PSendSysMessage(LANG_CHARACTER_DELETED_LIST_LINE_CHAT,
                itr->lowguid, itr->name.c_str(), itr->accountName.empty() ? "<nonexistent>" : itr->accountName.c_str(),
                itr->accountId, dateStr.c_str());
    }

    if (!m_session)
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_BAR);
}

/**
 * Handles the '.character deleted list' command, which shows all deleted characters which matches the given search string
 *
 * @see ChatHandler::HandleCharacterDeletedListHelper
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::DeletedInfoList
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedListCommand(char* args)
{
    DeletedInfoList foundList;
    if (!GetDeletedCharacterInfoList(foundList, args))
        return false;

    // if no characters have been found, output a warning
    if (foundList.empty())
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_EMPTY);
        return false;
    }

    HandleCharacterDeletedListHelper(foundList);
    return true;
}

/**
 * Restore a previously deleted character
 *
 * @see ChatHandler::HandleCharacterDeletedListHelper
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::DeletedInfoList
 *
 * @param delInfo the informations about the character which will be restored
 */
void ChatHandler::HandleCharacterDeletedRestoreHelper(DeletedInfo const& delInfo)
{
    if (delInfo.accountName.empty())                    // account not exist
    {
        PSendSysMessage(LANG_CHARACTER_DELETED_SKIP_ACCOUNT, delInfo.name.c_str(), delInfo.lowguid, delInfo.accountId);
        return;
    }

    // check character count
    uint32 charcount = sAccountMgr.GetCharactersCount(delInfo.accountId);
    if (charcount >= 10)
    {
        PSendSysMessage(LANG_CHARACTER_DELETED_SKIP_FULL, delInfo.name.c_str(), delInfo.lowguid, delInfo.accountId);
        return;
    }

    if (sObjectMgr.GetPlayerGuidByName(delInfo.name))
    {
        PSendSysMessage(LANG_CHARACTER_DELETED_SKIP_NAME, delInfo.name.c_str(), delInfo.lowguid, delInfo.accountId);
        return;
    }

    CharacterDatabase.PExecute("UPDATE characters SET name='%s', account='%u', deleteDate=NULL, deleteInfos_Name=NULL, deleteInfos_Account=NULL WHERE deleteDate IS NOT NULL AND guid = %u",
        delInfo.name.c_str(), delInfo.accountId, delInfo.lowguid);
}

/**
 * Handles the '.character deleted restore' command, which restores all deleted characters which matches the given search string
 *
 * The command automatically calls '.character deleted list' command with the search string to show all restored characters.
 *
 * @see ChatHandler::HandleCharacterDeletedRestoreHelper
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedRestoreCommand(char* args)
{
    // It is required to submit at least one argument
    if (!*args)
        return false;

    std::string searchString;
    std::string newCharName;
    uint32 newAccount = 0;

    // GCC by some strange reason fail build code without temporary variable
    std::istringstream params(args);
    params >> searchString >> newCharName >> newAccount;

    DeletedInfoList foundList;
    if (!GetDeletedCharacterInfoList(foundList, searchString))
        return false;

    if (foundList.empty())
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_EMPTY);
        return false;
    }

    SendSysMessage(LANG_CHARACTER_DELETED_RESTORE);
    HandleCharacterDeletedListHelper(foundList);

    if (newCharName.empty())
    {
        // Drop nonexistent account cases
        for (DeletedInfoList::iterator itr = foundList.begin(); itr != foundList.end(); ++itr)
            HandleCharacterDeletedRestoreHelper(*itr);
    }
    else if (foundList.size() == 1 && normalizePlayerName(newCharName))
    {
        DeletedInfo delInfo = foundList.front();

        // update name
        delInfo.name = newCharName;

        // if new account provided update deleted info
        if (newAccount && newAccount != delInfo.accountId)
        {
            delInfo.accountId = newAccount;
            sAccountMgr.GetName (newAccount, delInfo.accountName);
        }

        HandleCharacterDeletedRestoreHelper(delInfo);
    }
    else
        SendSysMessage(LANG_CHARACTER_DELETED_ERR_RENAME);

    return true;
}

/**
 * Handles the '.character deleted delete' command, which completely deletes all deleted characters which matches the given search string
 *
 * @see Player::GetDeletedCharacterGUIDs
 * @see Player::DeleteFromDB
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedDeleteCommand(char* args)
{
    // It is required to submit at least one argument
    if (!*args)
        return false;

    DeletedInfoList foundList;
    if (!GetDeletedCharacterInfoList(foundList, args))
        return false;

    if (foundList.empty())
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_EMPTY);
        return false;
    }

    SendSysMessage(LANG_CHARACTER_DELETED_DELETE);
    HandleCharacterDeletedListHelper(foundList);

    // Call the appropriate function to delete them (current account for deleted characters is 0)
    for(DeletedInfoList::const_iterator itr = foundList.begin(); itr != foundList.end(); ++itr)
        Player::DeleteFromDB(ObjectGuid(HIGHGUID_PLAYER, itr->lowguid), 0, false, true);

    return true;
}

/**
 * Handles the '.character deleted old' command, which completely deletes all deleted characters deleted with some days ago
 *
 * @see Player::DeleteOldCharacters
 * @see Player::DeleteFromDB
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedOldCommand(char* args)
{
    int32 keepDays = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_KEEP_DAYS);

    if (!ExtractOptInt32(&args, keepDays, sWorld.getConfig(CONFIG_UINT32_CHARDELETE_KEEP_DAYS)))
        return false;

    if (keepDays < 0)
        return false;

    Player::DeleteOldCharacters((uint32)keepDays);
    return true;
}

bool ChatHandler::HandleCharacterEraseCommand(char* args)
{
    char* nameStr = ExtractLiteralArg(&args);
    if (!nameStr)
        return false;

    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&nameStr, &target, &target_guid, &target_name))
        return false;

    uint32 account_id;

    if (target)
    {
        account_id = target->GetSession()->GetAccountId();
        target->GetSession()->KickPlayer();
    }
    else
        account_id = sObjectMgr.GetPlayerAccountIdByGUID(target_guid);

    std::string account_name;
    sAccountMgr.GetName (account_id,account_name);

    Player::DeleteFromDB(target_guid, account_id, true, true);
    PSendSysMessage(LANG_CHARACTER_DELETED, target_name.c_str(), target_guid.GetCounter(), account_name.c_str(), account_id);
    return true;
}

/// Close RA connection
bool ChatHandler::HandleQuitCommand(char* /*args*/)
{
    // processed in RASocket
    SendSysMessage(LANG_QUIT_WRONG_USE_ERROR);
    return true;
}

/// Exit the realm
bool ChatHandler::HandleServerExitCommand(char* /*args*/)
{
    SendSysMessage(LANG_COMMAND_EXIT);
    World::StopNow(SHUTDOWN_EXIT_CODE);
    return true;
}

/// Display info on users currently in the realm
bool ChatHandler::HandleAccountOnlineListCommand(char* args)
{
    uint32 limit;
    if (!ExtractOptUInt32(&args, limit, 100))
        return false;

    ///- Get the list of accounts ID logged to the realm
    //                                                 0   1         2        3        4
    QueryResult *result = LoginDatabase.PQuery("SELECT id, username, last_ip, gmlevel, expansion FROM account WHERE active_realm_id = %u", realmID);

    return ShowAccountListHelper(result,&limit);
}

/// Create an account
bool ChatHandler::HandleAccountCreateCommand(char* args)
{
    ///- %Parse the command line arguments
    char *szAcc = ExtractQuotedOrLiteralArg(&args);
    char *szPassword = ExtractQuotedOrLiteralArg(&args);
    if(!szAcc || !szPassword)
        return false;

    // normalized in accmgr.CreateAccount
    std::string account_name = szAcc;
    std::string password = szPassword;

    AccountOpResult result = sAccountMgr.CreateAccount(account_name, password);
    switch(result)
    {
        case AOR_OK:
            PSendSysMessage(LANG_ACCOUNT_CREATED,account_name.c_str());
            break;
        case AOR_NAME_TOO_LONG:
            SendSysMessage(LANG_ACCOUNT_TOO_LONG);
            SetSentErrorMessage(true);
            return false;
        case AOR_NAME_ALREDY_EXIST:
            SendSysMessage(LANG_ACCOUNT_ALREADY_EXIST);
            SetSentErrorMessage(true);
            return false;
        case AOR_DB_INTERNAL_ERROR:
            PSendSysMessage(LANG_ACCOUNT_NOT_CREATED_SQL_ERROR,account_name.c_str());
            SetSentErrorMessage(true);
            return false;
        default:
            PSendSysMessage(LANG_ACCOUNT_NOT_CREATED,account_name.c_str());
            SetSentErrorMessage(true);
            return false;
    }

    return true;
}

/// Set the filters of logging
bool ChatHandler::HandleServerLogFilterCommand(char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_LOG_FILTERS_STATE_HEADER);
        for(int i = 0; i < LOG_FILTER_COUNT; ++i)
            if (*logFilterData[i].name)
                PSendSysMessage("  %-20s = %s",logFilterData[i].name, sLog.HasLogFilter(1 << i) ? GetMangosString(LANG_ON) : GetMangosString(LANG_OFF));
        return true;
    }

    char *filtername = ExtractLiteralArg(&args);
    if (!filtername)
        return false;

    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    if (strncmp(filtername, "all", 4) == 0)
    {
        sLog.SetLogFilter(LogFilters(0xFFFFFFFF), value);
        PSendSysMessage(LANG_ALL_LOG_FILTERS_SET_TO_S, value ? GetMangosString(LANG_ON) : GetMangosString(LANG_OFF));
        return true;
    }

    for(int i = 0; i < LOG_FILTER_COUNT; ++i)
    {
        if (!*logFilterData[i].name)
            continue;

        if (!strncmp(filtername,logFilterData[i].name,strlen(filtername)))
        {
            sLog.SetLogFilter(LogFilters(1 << i),value);
            PSendSysMessage("  %-20s = %s",logFilterData[i].name,value ? GetMangosString(LANG_ON) : GetMangosString(LANG_OFF));
            return true;
        }
    }

    return false;
}

/// Set the level of logging
bool ChatHandler::HandleServerLogLevelCommand(char *args)
{
    if (!*args)
    {
        PSendSysMessage("Log level: %u", sLog.GetLogLevel());
        return true;
    }

    sLog.SetLogLevel(args);
    return true;
}

/// @}

#ifdef linux
// Non-blocking keypress detector, when return pressed, return 1, else always return 0
int kb_hit_return()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}
#endif

/// %Thread start
void CliRunnable::run()
{
    ///- Init new SQL thread for the world database (one connection call enough)
    WorldDatabase.ThreadStart();                                // let thread do safe mySQL requests

    char commandbuf[256];

    ///- Display the list of available CLI functions then beep
    sLog.outString();

    if (sConfig.GetBoolDefault("BeepAtStart", true))
        printf("\a");                                       // \a = Alert

    // print this here the first time
    // later it will be printed after command queue updates
    printf("mangos>");

    ///- As long as the World is running (no World::m_stopEvent), get the command line and handle it
    while (!World::IsStopped())
    {
        fflush(stdout);
        #ifdef linux
        while (!kb_hit_return() && !World::IsStopped())
            // With this, we limit CLI to 10commands/second
            usleep(100);
        if (World::IsStopped())
            break;
        #endif
        char *command_str = fgets(commandbuf,sizeof(commandbuf),stdin);
        if (command_str != NULL)
        {
            for(int x=0;command_str[x];x++)
                if(command_str[x]=='\r'||command_str[x]=='\n')
            {
                command_str[x]=0;
                break;
            }


            if(!*command_str)
            {
                printf("mangos>");
                continue;
            }

            std::string command;
            if(!consoleToUtf8(command_str,command))         // convert from console encoding to utf8
            {
                printf("mangos>");
                continue;
            }

            sWorld.QueueCliCommand(new CliCommandHolder(0, SEC_CONSOLE, NULL, command.c_str(), &utf8print, &commandFinished));
        }
        else if (feof(stdin))
        {
            World::StopNow(SHUTDOWN_EXIT_CODE);
        }
    }

    ///- End the database thread
    WorldDatabase.ThreadEnd();                                  // free mySQL thread resources
}
