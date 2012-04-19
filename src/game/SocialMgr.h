/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
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

#ifndef __MANGOS_SOCIALMGR_H
#define __MANGOS_SOCIALMGR_H

#include "Policies/Singleton.h"
#include "Database/DatabaseEnv.h"
#include "ObjectGuid.h"

class SocialMgr;
class PlayerSocial;
class Player;
class WorldPacket;

enum FriendStatus
{
    FRIEND_STATUS_OFFLINE   = 0,
    FRIEND_STATUS_ONLINE    = 1,
    FRIEND_STATUS_AFK       = 2,
    FRIEND_STATUS_UNK3      = 3,
    FRIEND_STATUS_DND       = 4
};

enum SocialFlag
{
    SOCIAL_FLAG_FRIEND      = 0x01,
    SOCIAL_FLAG_IGNORED     = 0x02,
    SOCIAL_FLAG_MUTED       = 0x04,                         // guessed
    SOCIAL_FLAG_RAF         = 0x08                          // Recruit-A-Friend
};

struct FriendInfo
{
    FriendStatus Status;
    uint32 Flags;
    uint32 Area;
    uint32 Level;
    uint32 Class;
    std::string Note;

    FriendInfo()
    {
        Status = FRIEND_STATUS_OFFLINE;
        Flags = 0;
        Area = 0;
        Level = 0;
        Class = 0;
        Note = "";
    }

    FriendInfo(uint32 flags, const std::string& note)
    {
        Status = FRIEND_STATUS_OFFLINE;
        Flags = flags;
        Area = 0;
        Level = 0;
        Class = 0;
        Note = note;
    }
};

typedef std::map<uint32, FriendInfo> PlayerSocialMap;
typedef std::map<uint32, PlayerSocial> SocialMap;

/// Results of friend related commands
enum FriendsResult
{
    FRIEND_DB_ERROR         = 0x00,                         // ERR_FRIEND_NOT_FOUND
    FRIEND_LIST_FULL        = 0x01,
    FRIEND_ONLINE           = 0x02,
    FRIEND_OFFLINE          = 0x03,
    FRIEND_NOT_FOUND        = 0x04,                         // ERR_FRIEND_NOT_FOUND
    FRIEND_REMOVED          = 0x05,
    FRIEND_ADDED_ONLINE     = 0x06,                         // ERR_FRIEND_ADDED_S
    FRIEND_ADDED_OFFLINE    = 0x07,
    FRIEND_ALREADY          = 0x08,
    FRIEND_SELF             = 0x09,
    FRIEND_ENEMY            = 0x0A,
    FRIEND_IGNORE_FULL      = 0x0B,
    FRIEND_IGNORE_SELF      = 0x0C,
    FRIEND_IGNORE_NOT_FOUND = 0x0D,
    FRIEND_IGNORE_ALREADY   = 0x0E,
    FRIEND_IGNORE_ADDED     = 0x0F,
    FRIEND_IGNORE_REMOVED   = 0x10,
    FRIEND_IGNORE_AMBIGUOUS = 0x11,                         // That name is ambiguous, type more of the player's server name
    FRIEND_MUTE_FULL        = 0x12,
    FRIEND_MUTE_SELF        = 0x13,
    FRIEND_MUTE_NOT_FOUND   = 0x14,
    FRIEND_MUTE_ALREADY     = 0x15,
    FRIEND_MUTE_ADDED       = 0x16,
    FRIEND_MUTE_REMOVED     = 0x17,
    FRIEND_MUTE_AMBIGUOUS   = 0x18,                         // ERR_VOICE_IGNORE_AMBIGUOUS
    FRIEND_UNK7             = 0x19,                         // ERR_MAX_VALUE (nothing is showed)
    FRIEND_UNKNOWN          = 0x1A                          // Unknown friend response from server
};

#define SOCIALMGR_FRIEND_LIMIT  50
#define SOCIALMGR_IGNORE_LIMIT  50

class PlayerSocial
{
    friend class SocialMgr;
    public:
        PlayerSocial();
        ~PlayerSocial();
        // adding/removing
        bool AddToSocialList(ObjectGuid friend_guid, bool ignore);
        void RemoveFromSocialList(ObjectGuid friend_guid, bool ignore);
        void SetFriendNote(ObjectGuid friend_guid, std::string note);
        // Packet send's
        void SendSocialList();
        // Misc
        bool HasFriend(ObjectGuid friend_guid);
        bool HasIgnore(ObjectGuid ignore_guid);
        void SetPlayerGuid(ObjectGuid guid) { m_playerLowGuid = guid.GetCounter(); }
        uint32 GetNumberOfSocialsWithFlag(SocialFlag flag);
    private:
        PlayerSocialMap m_playerSocialMap;
        uint32 m_playerLowGuid;
};

class SocialMgr
{
    public:
        SocialMgr();
        ~SocialMgr();
        // Misc
        void RemovePlayerSocial(uint32 guid) { m_socialMap.erase(guid); }

        void GetFriendInfo(Player *player, uint32 friendGUID, FriendInfo &friendInfo);
        // Packet management
        void MakeFriendStatusPacket(FriendsResult result, uint32 friend_guid, WorldPacket *data);
        void SendFriendStatus(Player *player, FriendsResult result, ObjectGuid friend_guid, bool broadcast);
        void BroadcastToFriendListers(Player *player, WorldPacket *packet);
        // Loading
        PlayerSocial *LoadFromDB(QueryResult *result, ObjectGuid guid);
    private:
        SocialMap m_socialMap;
};

#define sSocialMgr MaNGOS::Singleton<SocialMgr>::Instance()
#endif
