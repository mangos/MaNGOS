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

#include "Common.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "Group.h"
#include "Formulas.h"
#include "ObjectAccessor.h"
#include "BattleGround.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "Util.h"
#include "LootMgr.h"

#define LOOT_ROLL_TIMEOUT  (1*MINUTE*IN_MILLISECONDS)

//===================================================
//============== Roll ===============================
//===================================================

void Roll::targetObjectBuildLink()
{
    // called from link()
    getTarget()->addLootValidatorRef(this);
}

void Roll::CalculateCommonVoteMask(uint32 max_enchanting_skill)
{
    m_commonVoteMask = ROLL_VOTE_MASK_ALL;

    ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(itemid);

    if (itemProto->Flags2 & ITEM_FLAG2_NEED_ROLL_DISABLED)
        m_commonVoteMask = RollVoteMask(m_commonVoteMask & ~ROLL_VOTE_MASK_NEED);

    if (!itemProto->DisenchantID || uint32(itemProto->RequiredDisenchantSkill) > max_enchanting_skill)
        m_commonVoteMask = RollVoteMask(m_commonVoteMask & ~ROLL_VOTE_MASK_DISENCHANT);
}

RollVoteMask Roll::GetVoteMaskFor(Player* player) const
{
    ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(itemid);

    // In NEED_BEFORE_GREED need disabled for non-usable item for player
    if (m_method != NEED_BEFORE_GREED || player->CanUseItem(itemProto) == EQUIP_ERR_OK)
        return m_commonVoteMask;
    else
        return RollVoteMask(m_commonVoteMask & ~ROLL_VOTE_MASK_NEED);
}

//===================================================
//============== Group ==============================
//===================================================

Group::Group() : m_Id(0), m_groupType(GROUPTYPE_NORMAL),
    m_dungeonDifficulty(REGULAR_DIFFICULTY), m_raidDifficulty(REGULAR_DIFFICULTY),
    m_bgGroup(NULL), m_lootMethod(FREE_FOR_ALL), m_lootThreshold(ITEM_QUALITY_UNCOMMON),
    m_subGroupsCounts(NULL)
{
}

Group::~Group()
{
    if(m_bgGroup)
    {
        DEBUG_LOG("Group::~Group: battleground group being deleted.");
        if(m_bgGroup->GetBgRaid(ALLIANCE) == this)
            m_bgGroup->SetBgRaid(ALLIANCE, NULL);
        else if(m_bgGroup->GetBgRaid(HORDE) == this)
            m_bgGroup->SetBgRaid(HORDE, NULL);
        else
            sLog.outError("Group::~Group: battleground group is not linked to the correct battleground.");
    }
    Rolls::iterator itr;
    while(!RollId.empty())
    {
        itr = RollId.begin();
        Roll *r = *itr;
        RollId.erase(itr);
        delete(r);
    }

    // it is undefined whether objectmgr (which stores the groups) or instancesavemgr
    // will be unloaded first so we must be prepared for both cases
    // this may unload some dungeon persistent state
    for(uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        for(BoundInstancesMap::iterator itr2 = m_boundInstances[i].begin(); itr2 != m_boundInstances[i].end(); ++itr2)
            itr2->second.state->RemoveGroup(this);

    // Sub group counters clean up
    if (m_subGroupsCounts)
        delete[] m_subGroupsCounts;
}

bool Group::Create(ObjectGuid guid, const char * name)
{
    m_leaderGuid = guid;
    m_leaderName = name;

    m_groupType  = isBGGroup() ? GROUPTYPE_BGRAID : GROUPTYPE_NORMAL;

    if (m_groupType & GROUPTYPE_RAID)
        _initRaidSubGroupsCounter();

    m_lootMethod = GROUP_LOOT;
    m_lootThreshold = ITEM_QUALITY_UNCOMMON;
    m_looterGuid = guid;

    m_dungeonDifficulty = DUNGEON_DIFFICULTY_NORMAL;
    m_raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;
    if (!isBGGroup())
    {
        m_Id = sObjectMgr.GenerateGroupLowGuid();

        Player *leader = sObjectMgr.GetPlayer(guid);
        if(leader)
        {
            m_dungeonDifficulty = leader->GetDungeonDifficulty();
            m_raidDifficulty = leader->GetRaidDifficulty();
        }

        Player::ConvertInstancesToGroup(leader, this, guid);

        // store group in database
        CharacterDatabase.BeginTransaction();
        CharacterDatabase.PExecute("DELETE FROM groups WHERE groupId ='%u'", m_Id);
        CharacterDatabase.PExecute("DELETE FROM group_member WHERE groupId ='%u'", m_Id);
        CharacterDatabase.PExecute("INSERT INTO groups (groupId,leaderGuid,mainTank,mainAssistant,lootMethod,looterGuid,lootThreshold,icon1,icon2,icon3,icon4,icon5,icon6,icon7,icon8,groupType,difficulty,raiddifficulty) "
            "VALUES ('%u','%u','%u','%u','%u','%u','%u','" UI64FMTD "','" UI64FMTD "','" UI64FMTD "','" UI64FMTD "','" UI64FMTD "','" UI64FMTD "','" UI64FMTD "','" UI64FMTD "','%u','%u','%u')",
            m_Id, m_leaderGuid.GetCounter(), m_mainTankGuid.GetCounter(), m_mainAssistantGuid.GetCounter(), uint32(m_lootMethod),
            m_looterGuid.GetCounter(), uint32(m_lootThreshold),
            m_targetIcons[0].GetRawValue(), m_targetIcons[1].GetRawValue(),
            m_targetIcons[2].GetRawValue(), m_targetIcons[3].GetRawValue(),
            m_targetIcons[4].GetRawValue(), m_targetIcons[5].GetRawValue(),
            m_targetIcons[6].GetRawValue(), m_targetIcons[7].GetRawValue(),
            uint8(m_groupType), uint32(m_dungeonDifficulty), uint32(m_raidDifficulty));
    }

    if (!AddMember(guid, name))
        return false;

    if (!isBGGroup())
        CharacterDatabase.CommitTransaction();

    return true;
}

bool Group::LoadGroupFromDB(Field* fields)
{
    //                                          0         1              2           3           4              5      6      7      8      9      10     11     12     13         14          15              16          17
    // result = CharacterDatabase.Query("SELECT mainTank, mainAssistant, lootMethod, looterGuid, lootThreshold, icon1, icon2, icon3, icon4, icon5, icon6, icon7, icon8, groupType, difficulty, raiddifficulty, leaderGuid, groupId FROM groups");

    m_Id = fields[17].GetUInt32();
    m_leaderGuid = ObjectGuid(HIGHGUID_PLAYER, fields[16].GetUInt32());

    // group leader not exist
    if (!sObjectMgr.GetPlayerNameByGUID(m_leaderGuid, m_leaderName))
        return false;

    m_groupType  = GroupType(fields[13].GetUInt8());

    if (m_groupType & GROUPTYPE_RAID)
        _initRaidSubGroupsCounter();

    uint32 diff = fields[14].GetUInt8();
    if (diff >= MAX_DUNGEON_DIFFICULTY)
        diff = DUNGEON_DIFFICULTY_NORMAL;
    m_dungeonDifficulty = Difficulty(diff);

    uint32 r_diff = fields[15].GetUInt8();
    if (r_diff >= MAX_RAID_DIFFICULTY)
        r_diff = RAID_DIFFICULTY_10MAN_NORMAL;
    m_raidDifficulty = Difficulty(r_diff);

    m_mainTankGuid = ObjectGuid(HIGHGUID_PLAYER, fields[0].GetUInt32());
    m_mainAssistantGuid = ObjectGuid(HIGHGUID_PLAYER, fields[1].GetUInt32());
    m_lootMethod = LootMethod(fields[2].GetUInt8());
    m_looterGuid = ObjectGuid(HIGHGUID_PLAYER, fields[3].GetUInt32());
    m_lootThreshold = ItemQualities(fields[4].GetUInt16());

    for(int i = 0; i < TARGET_ICON_COUNT; ++i)
        m_targetIcons[i] = ObjectGuid(fields[5+i].GetUInt64());

    return true;
}

bool Group::LoadMemberFromDB(uint32 guidLow, uint8 subgroup, bool assistant)
{
    MemberSlot member;
    member.guid      = ObjectGuid(HIGHGUID_PLAYER, guidLow);

    // skip nonexistent member
    if (!sObjectMgr.GetPlayerNameByGUID(member.guid, member.name))
        return false;

    member.group     = subgroup;
    member.assistant = assistant;
    m_memberSlots.push_back(member);

    SubGroupCounterIncrease(subgroup);

    return true;
}

void Group::ConvertToRaid()
{
    m_groupType = GroupType(m_groupType | GROUPTYPE_RAID);

    _initRaidSubGroupsCounter();

    if(!isBGGroup())
        CharacterDatabase.PExecute("UPDATE groups SET groupType = %u WHERE groupId='%u'", uint8(m_groupType), m_Id);
    SendUpdate();

    // update quest related GO states (quest activity dependent from raid membership)
    for(member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
        if(Player* player = sObjectMgr.GetPlayer(citr->guid))
            player->UpdateForQuestWorldObjects();
}

bool Group::AddInvite(Player *player)
{
    if( !player || player->GetGroupInvite() )
        return false;
    Group* group = player->GetGroup();
    if( group && group->isBGGroup() )
        group = player->GetOriginalGroup();
    if( group )
        return false;

    RemoveInvite(player);

    m_invitees.insert(player);

    player->SetGroupInvite(this);

    return true;
}

bool Group::AddLeaderInvite(Player *player)
{
    if(!AddInvite(player))
        return false;

    m_leaderGuid = player->GetObjectGuid();
    m_leaderName = player->GetName();
    return true;
}

uint32 Group::RemoveInvite(Player *player)
{
    m_invitees.erase(player);

    player->SetGroupInvite(NULL);
    return GetMembersCount();
}

void Group::RemoveAllInvites()
{
    for(InvitesList::iterator itr = m_invitees.begin(); itr!=m_invitees.end(); ++itr)
        (*itr)->SetGroupInvite(NULL);

    m_invitees.clear();
}

Player* Group::GetInvited(ObjectGuid guid) const
{
    for(InvitesList::const_iterator itr = m_invitees.begin(); itr != m_invitees.end(); ++itr)
        if ((*itr)->GetObjectGuid() == guid)
            return (*itr);

    return NULL;
}

Player* Group::GetInvited(const std::string& name) const
{
    for(InvitesList::const_iterator itr = m_invitees.begin(); itr != m_invitees.end(); ++itr)
    {
        if((*itr)->GetName() == name)
            return (*itr);
    }
    return NULL;
}

bool Group::AddMember(ObjectGuid guid, const char* name)
{
    if (!_addMember(guid, name))
        return false;

    SendUpdate();

    if (Player* player = sObjectMgr.GetPlayer(guid))
    {
        if (!IsLeader(player->GetObjectGuid()) && !isBGGroup())
        {
            // reset the new member's instances, unless he is currently in one of them
            // including raid/heroic instances that they are not permanently bound to!
            player->ResetInstances(INSTANCE_RESET_GROUP_JOIN,false);
            player->ResetInstances(INSTANCE_RESET_GROUP_JOIN,true);

            if (player->getLevel() >= LEVELREQUIREMENT_HEROIC)
            {
                if (player->GetDungeonDifficulty() != GetDungeonDifficulty())
                {
                    player->SetDungeonDifficulty(GetDungeonDifficulty());
                    player->SendDungeonDifficulty(true);
                }
                if (player->GetRaidDifficulty() != GetRaidDifficulty())
                {
                    player->SetRaidDifficulty(GetRaidDifficulty());
                    player->SendRaidDifficulty(true);
                }
            }
        }
        player->SetGroupUpdateFlag(GROUP_UPDATE_FULL);
        UpdatePlayerOutOfRange(player);

        // quest related GO state dependent from raid membership
        if(isRaidGroup())
            player->UpdateForQuestWorldObjects();
    }

    return true;
}

uint32 Group::RemoveMember(ObjectGuid guid, uint8 method)
{
    // remove member and change leader (if need) only if strong more 2 members _before_ member remove
    if (GetMembersCount() > uint32(isBGGroup() ? 1 : 2))    // in BG group case allow 1 members group
    {
        bool leaderChanged = _removeMember(guid);

        if (Player *player = sObjectMgr.GetPlayer( guid ))
        {
            // quest related GO state dependent from raid membership
            if (isRaidGroup())
                player->UpdateForQuestWorldObjects();

            WorldPacket data;

            if (method == 1)
            {
                data.Initialize( SMSG_GROUP_UNINVITE, 0 );
                player->GetSession()->SendPacket( &data );
            }

            //we already removed player from group and in player->GetGroup() is his original group!
            if (Group* group = player->GetGroup())
            {
                group->SendUpdate();
            }
            else
            {
                data.Initialize(SMSG_GROUP_LIST, 1+1+1+1+8+4+4+8);
                data << uint8(0x10) << uint8(0) << uint8(0) << uint8(0);
                data << uint64(0) << uint32(0) << uint32(0) << uint64(0);
                player->GetSession()->SendPacket(&data);
            }

            _homebindIfInstance(player);
        }

        if (leaderChanged)
        {
            WorldPacket data(SMSG_GROUP_SET_LEADER, (m_memberSlots.front().name.size()+1));
            data << m_memberSlots.front().name;
            BroadcastPacket(&data, true);
        }

        SendUpdate();
    }
    // if group before remove <= 2 disband it
    else
        Disband(true);

    return m_memberSlots.size();
}

void Group::ChangeLeader(ObjectGuid guid)
{
    member_citerator slot = _getMemberCSlot(guid);
    if (slot == m_memberSlots.end())
        return;

    _setLeader(guid);

    WorldPacket data(SMSG_GROUP_SET_LEADER, slot->name.size()+1);
    data << slot->name;
    BroadcastPacket(&data, true);
    SendUpdate();
}

void Group::Disband(bool hideDestroy)
{
    Player *player;

    for(member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        player = sObjectMgr.GetPlayer(citr->guid);
        if(!player)
            continue;

        //we cannot call _removeMember because it would invalidate member iterator
        //if we are removing player from battleground raid
        if( isBGGroup() )
            player->RemoveFromBattleGroundRaid();
        else
        {
            //we can remove player who is in battleground from his original group
            if( player->GetOriginalGroup() == this )
                player->SetOriginalGroup(NULL);
            else
                player->SetGroup(NULL);
        }

        // quest related GO state dependent from raid membership
        if(isRaidGroup())
            player->UpdateForQuestWorldObjects();

        if(!player->GetSession())
            continue;

        WorldPacket data;
        if(!hideDestroy)
        {
            data.Initialize(SMSG_GROUP_DESTROYED, 0);
            player->GetSession()->SendPacket(&data);
        }

        //we already removed player from group and in player->GetGroup() is his original group, send update
        if( Group* group = player->GetGroup() )
        {
            group->SendUpdate();
        }
        else
        {
            data.Initialize(SMSG_GROUP_LIST, 1+1+1+1+8+4+4+8);
            data << uint8(0x10) << uint8(0) << uint8(0) << uint8(0);
            data << uint64(0) << uint32(0) << uint32(0) << uint64(0);
            player->GetSession()->SendPacket(&data);
        }

        _homebindIfInstance(player);
    }
    RollId.clear();
    m_memberSlots.clear();

    RemoveAllInvites();

    if(!isBGGroup())
    {
        CharacterDatabase.BeginTransaction();
        CharacterDatabase.PExecute("DELETE FROM groups WHERE groupId='%u'", m_Id);
        CharacterDatabase.PExecute("DELETE FROM group_member WHERE groupId='%u'", m_Id);
        CharacterDatabase.CommitTransaction();
        ResetInstances(INSTANCE_RESET_GROUP_DISBAND, false, NULL);
        ResetInstances(INSTANCE_RESET_GROUP_DISBAND, true, NULL);
    }

    m_leaderGuid.Clear();
    m_leaderName = "";
}

/*********************************************************/
/***                   LOOT SYSTEM                     ***/
/*********************************************************/

void Group::SendLootStartRoll(uint32 CountDown, uint32 mapid, const Roll &r)
{
    WorldPacket data(SMSG_LOOT_START_ROLL, (8+4+4+4+4+4+4+1));
    data << r.lootedTargetGUID;                             // creature guid what we're looting
    data << uint32(mapid);                                  // 3.3.3 mapid
    data << uint32(r.itemSlot);                             // item slot in loot
    data << uint32(r.itemid);                               // the itemEntryId for the item that shall be rolled for
    data << uint32(r.itemRandomSuffix);                     // randomSuffix
    data << uint32(r.itemRandomPropId);                     // item random property ID
    data << uint32(r.itemCount);                            // items in stack
    data << uint32(CountDown);                              // the countdown time to choose "need" or "greed"

    size_t voteMaskPos = data.wpos();
    data << uint8(0);                                       // roll type mask, allowed choices (placeholder)

    for (Roll::PlayerVote::const_iterator itr = r.playerVote.begin(); itr != r.playerVote.end(); ++itr)
    {
        Player *p = sObjectMgr.GetPlayer(itr->first);
        if(!p || !p->GetSession())
            continue;

        if(itr->second == ROLL_NOT_VALID)
            continue;

        // dependent from player
        RollVoteMask mask = r.GetVoteMaskFor(p);
        data.put<uint8>(voteMaskPos,uint8(mask));

        p->GetSession()->SendPacket( &data );
    }
}

void Group::SendLootRoll(ObjectGuid const& targetGuid, uint8 rollNumber, uint8 rollType, const Roll &r)
{
    WorldPacket data(SMSG_LOOT_ROLL, (8+4+8+4+4+4+1+1+1));
    data << r.lootedTargetGUID;                             // creature guid what we're looting
    data << uint32(r.itemSlot);                             // unknown, maybe amount of players, or item slot in loot
    data << targetGuid;
    data << uint32(r.itemid);                               // the itemEntryId for the item that shall be rolled for
    data << uint32(r.itemRandomSuffix);                     // randomSuffix
    data << uint32(r.itemRandomPropId);                     // Item random property ID
    data << uint8(rollNumber);                              // 0: "Need for: [item name]" > 127: "you passed on: [item name]"      Roll number
    data << uint8(rollType);                                // 0: "Need for: [item name]" 0: "You have selected need for [item name] 1: need roll 2: greed roll
    data << uint8(0);                                       // auto pass on loot

    for( Roll::PlayerVote::const_iterator itr = r.playerVote.begin(); itr != r.playerVote.end(); ++itr)
    {
        Player *p = sObjectMgr.GetPlayer(itr->first);
        if(!p || !p->GetSession())
            continue;

        if(itr->second != ROLL_NOT_VALID)
            p->GetSession()->SendPacket( &data );
    }
}

void Group::SendLootRollWon(ObjectGuid const& targetGuid, uint8 rollNumber, RollVote rollType, const Roll &r)
{
    WorldPacket data(SMSG_LOOT_ROLL_WON, (8+4+4+4+4+8+1+1));
    data << r.lootedTargetGUID;                             // creature guid what we're looting
    data << uint32(r.itemSlot);                             // item slot in loot
    data << uint32(r.itemid);                               // the itemEntryId for the item that shall be rolled for
    data << uint32(r.itemRandomSuffix);                     // randomSuffix
    data << uint32(r.itemRandomPropId);                     // Item random property
    data << targetGuid;                                     // guid of the player who won.
    data << uint8(rollNumber);                              // rollnumber related to SMSG_LOOT_ROLL
    data << uint8(rollType);                                // Rolltype related to SMSG_LOOT_ROLL

    for( Roll::PlayerVote::const_iterator itr = r.playerVote.begin(); itr != r.playerVote.end(); ++itr)
    {
        Player *p = sObjectMgr.GetPlayer(itr->first);
        if(!p || !p->GetSession())
            continue;

        if(itr->second != ROLL_NOT_VALID)
            p->GetSession()->SendPacket( &data );
    }
}

void Group::SendLootAllPassed(Roll const& r)
{
    WorldPacket data(SMSG_LOOT_ALL_PASSED, (8+4+4+4+4));
    data << r.lootedTargetGUID;                             // creature guid what we're looting
    data << uint32(r.itemSlot);                             // item slot in loot
    data << uint32(r.itemid);                               // The itemEntryId for the item that shall be rolled for
    data << uint32(r.itemRandomPropId);                     // Item random property ID
    data << uint32(r.itemRandomSuffix);                     // Item random suffix ID

    for( Roll::PlayerVote::const_iterator itr=r.playerVote.begin(); itr!=r.playerVote.end(); ++itr)
    {
        Player *p = sObjectMgr.GetPlayer(itr->first);
        if(!p || !p->GetSession())
            continue;

        if(itr->second != ROLL_NOT_VALID)
            p->GetSession()->SendPacket( &data );
    }
}

void Group::GroupLoot(WorldObject* pSource, Loot* loot)
{
    uint32 maxEnchantingSkill = GetMaxSkillValueForGroup(SKILL_ENCHANTING);

    for(uint8 itemSlot = 0; itemSlot < loot->items.size(); ++itemSlot)
    {
        LootItem& lootItem = loot->items[itemSlot];
        ItemPrototype const *itemProto = ObjectMgr::GetItemPrototype(lootItem.itemid);
        if (!itemProto)
        {
            DEBUG_LOG("Group::GroupLoot: missing item prototype for item with id: %d", lootItem.itemid);
            continue;
        }

        //roll for over-threshold item if it's one-player loot
        if (itemProto->Quality >= uint32(m_lootThreshold) && !lootItem.freeforall)
            StartLootRoll(pSource, GROUP_LOOT, loot, itemSlot, maxEnchantingSkill);
        else
            lootItem.is_underthreshold = 1;
    }
}

void Group::NeedBeforeGreed(WorldObject* pSource, Loot* loot)
{
    uint32 maxEnchantingSkill = GetMaxSkillValueForGroup(SKILL_ENCHANTING);

    for(uint8 itemSlot = 0; itemSlot < loot->items.size(); ++itemSlot)
    {
        LootItem& lootItem = loot->items[itemSlot];
        ItemPrototype const *itemProto = ObjectMgr::GetItemPrototype(lootItem.itemid);
        if (!itemProto)
        {
            DEBUG_LOG("Group::NeedBeforeGreed: missing item prototype for item with id: %d", lootItem.itemid);
            continue;
        }

        //only roll for one-player items, not for ones everyone can get
        if (itemProto->Quality >= uint32(m_lootThreshold) && !lootItem.freeforall)
            StartLootRoll(pSource, NEED_BEFORE_GREED, loot, itemSlot, maxEnchantingSkill);
        else
            lootItem.is_underthreshold = 1;
    }
}

void Group::MasterLoot(WorldObject* pSource, Loot* loot)
{
    for (LootItemList::iterator i=loot->items.begin(); i != loot->items.end(); ++i)
    {
        ItemPrototype const *item = ObjectMgr::GetItemPrototype(i->itemid);
        if (!item)
            continue;
        if (item->Quality < uint32(m_lootThreshold))
            i->is_underthreshold = 1;
    }

    uint32 real_count = 0;

    WorldPacket data(SMSG_LOOT_MASTER_LIST, 330);
    data << uint8(GetMembersCount());

    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *looter = itr->getSource();
        if (!looter->IsInWorld())
            continue;

        if (looter->IsWithinDist(pSource, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE), false))
        {
            data << looter->GetObjectGuid();
            ++real_count;
        }
    }

    data.put<uint8>(0, real_count);

    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *looter = itr->getSource();
        if (looter->IsWithinDist(pSource, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE), false))
            looter->GetSession()->SendPacket(&data);
    }
}

bool Group::CountRollVote(Player* player, ObjectGuid const& lootedTarget, uint32 itemSlot, RollVote vote)
{
    Rolls::iterator rollI = RollId.begin();
    for (; rollI != RollId.end(); ++rollI)
        if ((*rollI)->isValid() && (*rollI)->lootedTargetGUID == lootedTarget && (*rollI)->itemSlot == itemSlot)
            break;

    if (rollI == RollId.end())
        return false;

    // possible cheating
    RollVoteMask voteMask = (*rollI)->GetVoteMaskFor(player);
    if ((voteMask & (1 << vote)) == 0)
        return false;

    CountRollVote(player->GetObjectGuid(), rollI, vote);    // result not related this function result meaning, ignore
    return true;
}

bool Group::CountRollVote(ObjectGuid const& playerGUID, Rolls::iterator& rollI, RollVote vote)
{
    Roll* roll = *rollI;

    Roll::PlayerVote::iterator itr = roll->playerVote.find(playerGUID);
    // this condition means that player joins to the party after roll begins
    if (itr == roll->playerVote.end())
        return true;                                        // result used for need iterator ++, so avoid for end of list

    if (roll->getLoot())
        if (roll->getLoot()->items.empty())
            return false;

    switch (vote)
    {
        case ROLL_PASS:                                     // Player choose pass
        {
            SendLootRoll(playerGUID, 128, 128, *roll);
            ++roll->totalPass;
            itr->second = ROLL_PASS;
            break;
        }
        case ROLL_NEED:                                     // player choose Need
        {
            SendLootRoll(playerGUID, 0, 0, *roll);
            ++roll->totalNeed;
            itr->second = ROLL_NEED;
            break;
        }
        case ROLL_GREED:                                    // player choose Greed
        {
            SendLootRoll(playerGUID, 128, ROLL_GREED, *roll);
            ++roll->totalGreed;
            itr->second = ROLL_GREED;
            break;
        }
        case ROLL_DISENCHANT:                               // player choose Disenchant
        {
            SendLootRoll(playerGUID, 128, ROLL_DISENCHANT, *roll);
            ++roll->totalGreed;
            itr->second = ROLL_DISENCHANT;
            break;
        }
        default:                                            // Roll removed case
            break;
    }

    if (roll->totalPass + roll->totalNeed + roll->totalGreed >= roll->totalPlayersRolling)
    {
        CountTheRoll(rollI);
        return true;
    }

    return false;
}

void Group::StartLootRoll(WorldObject* lootTarget, LootMethod method, Loot* loot, uint8 itemSlot, uint32 maxEnchantingSkill)
{
    if (itemSlot >= loot->items.size())
        return;

    LootItem const& lootItem = loot->items[itemSlot];

    Roll* r = new Roll(lootTarget->GetObjectGuid(), method, lootItem);

    //a vector is filled with only near party members
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* playerToRoll = itr->getSource();
        if (!playerToRoll || !playerToRoll->GetSession())
            continue;

        if (lootItem.AllowedForPlayer(playerToRoll))
        {
            if (playerToRoll->IsWithinDistInMap(lootTarget, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE), false))
            {
                r->playerVote[playerToRoll->GetObjectGuid()] = ROLL_NOT_EMITED_YET;
                ++r->totalPlayersRolling;
            }
        }
    }

    if (r->totalPlayersRolling > 0)                         // has looters
    {
        r->setLoot(loot);
        r->itemSlot = itemSlot;

        if (r->totalPlayersRolling == 1)                    // single looter
            r->playerVote.begin()->second = ROLL_NEED;
        else
        {
            // Only GO-group looting and NPC-group looting possible
            MANGOS_ASSERT(lootTarget->isType(TYPEMASK_CREATURE_OR_GAMEOBJECT));

            r->CalculateCommonVoteMask(maxEnchantingSkill); // dependent from item and possible skill

            SendLootStartRoll(LOOT_ROLL_TIMEOUT, lootTarget->GetMapId(), *r);
            loot->items[itemSlot].is_blocked = true;

            lootTarget->StartGroupLoot(this, LOOT_ROLL_TIMEOUT);
        }

        RollId.push_back(r);
    }
    else                                            // no looters??
        delete r;
}

// called when roll timer expires
void Group::EndRoll()
{
    while(!RollId.empty())
    {
        //need more testing here, if rolls disappear
        Rolls::iterator itr = RollId.begin();
        CountTheRoll(itr);                                  //i don't have to edit player votes, who didn't vote ... he will pass
    }
}

void Group::CountTheRoll(Rolls::iterator& rollI)
{
    Roll* roll = *rollI;
    if(!roll->isValid())                                    // is loot already deleted ?
    {
        rollI = RollId.erase(rollI);
        delete roll;
        return;
    }

    //end of the roll
    if (roll->totalNeed > 0)
    {
        if(!roll->playerVote.empty())
        {
            uint8 maxresul = 0;
            ObjectGuid maxguid  = (*roll->playerVote.begin()).first;
            Player *player;

            for(Roll::PlayerVote::const_iterator itr = roll->playerVote.begin(); itr != roll->playerVote.end(); ++itr)
            {
                if (itr->second != ROLL_NEED)
                    continue;

                uint8 randomN = urand(1, 100);
                SendLootRoll(itr->first, randomN, ROLL_NEED, *roll);
                if (maxresul < randomN)
                {
                    maxguid  = itr->first;
                    maxresul = randomN;
                }
            }
            SendLootRollWon(maxguid, maxresul, ROLL_NEED, *roll);
            player = sObjectMgr.GetPlayer(maxguid);

            if(player && player->GetSession())
            {
                player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT, roll->itemid, maxresul);

                ItemPosCountVec dest;
                LootItem *item = &(roll->getLoot()->items[roll->itemSlot]);
                InventoryResult msg = player->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, roll->itemid, item->count );
                if ( msg == EQUIP_ERR_OK )
                {
                    item->is_looted = true;
                    roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                    --roll->getLoot()->unlootedCount;
                    player->StoreNewItem( dest, roll->itemid, true, item->randomPropertyId);
                    player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM, roll->itemid, item->count);
                    player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE, roll->getLoot()->loot_type, item->count);
                    player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM, roll->itemid, item->count);
                }
                else
                {
                    item->is_blocked = false;
                    player->SendEquipError( msg, NULL, NULL, roll->itemid );
                }
            }
        }
    }
    else if (roll->totalGreed > 0)
    {
        if(!roll->playerVote.empty())
        {
            uint8 maxresul = 0;
            ObjectGuid maxguid = (*roll->playerVote.begin()).first;
            Player *player;
            RollVote rollvote = ROLL_PASS;                  //Fixed: Using uninitialized memory 'rollvote'

            Roll::PlayerVote::iterator itr;
            for (itr = roll->playerVote.begin(); itr != roll->playerVote.end(); ++itr)
            {
                if (itr->second != ROLL_GREED && itr->second != ROLL_DISENCHANT)
                    continue;

                uint8 randomN = urand(1, 100);
                SendLootRoll(itr->first, randomN, itr->second, *roll);
                if (maxresul < randomN)
                {
                    maxguid  = itr->first;
                    maxresul = randomN;
                    rollvote = itr->second;
                }
            }
            SendLootRollWon(maxguid, maxresul, rollvote, *roll);
            player = sObjectMgr.GetPlayer(maxguid);

            if(player && player->GetSession())
            {
                player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT, roll->itemid, maxresul);

                LootItem *item = &(roll->getLoot()->items[roll->itemSlot]);

                if(rollvote == ROLL_GREED)
                {
                    ItemPosCountVec dest;
                    InventoryResult msg = player->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, roll->itemid, item->count );
                    if ( msg == EQUIP_ERR_OK )
                    {
                        item->is_looted = true;
                        roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                        --roll->getLoot()->unlootedCount;
                        player->StoreNewItem( dest, roll->itemid, true, item->randomPropertyId);
                        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM, roll->itemid, item->count);
                        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE, roll->getLoot()->loot_type, item->count);
                        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM, roll->itemid, item->count);
                    }
                    else
                    {
                        item->is_blocked = false;
                        player->SendEquipError( msg, NULL, NULL, roll->itemid );
                    }
                }
                else if(rollvote == ROLL_DISENCHANT)
                {
                    item->is_looted = true;
                    roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                    --roll->getLoot()->unlootedCount;

                    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(roll->itemid);
                    player->AutoStoreLoot(pProto->DisenchantID, LootTemplates_Disenchant, true);
                }
            }
        }
    }
    else
    {
        SendLootAllPassed(*roll);
        LootItem *item = &(roll->getLoot()->items[roll->itemSlot]);
        if(item) item->is_blocked = false;
    }
    rollI = RollId.erase(rollI);
    delete roll;
}

void Group::SetTargetIcon(uint8 id, ObjectGuid whoGuid, ObjectGuid targetGuid)
{
    if (id >= TARGET_ICON_COUNT)
        return;

    // clean other icons
    if (targetGuid)
        for(int i = 0; i < TARGET_ICON_COUNT; ++i)
            if (m_targetIcons[i] == targetGuid)
                SetTargetIcon(i, ObjectGuid(), ObjectGuid());

    m_targetIcons[id] = targetGuid;

    WorldPacket data(MSG_RAID_TARGET_UPDATE, (1+8+1+8));
    data << uint8(0);                                       // set targets
    data << whoGuid;
    data << uint8(id);
    data << targetGuid;
    BroadcastPacket(&data, true);
}

static void GetDataForXPAtKill_helper(Player* player, Unit const* victim, uint32& sum_level, Player* & member_with_max_level, Player* & not_gray_member_with_max_level)
{
    sum_level += player->getLevel();
    if(!member_with_max_level || member_with_max_level->getLevel() < player->getLevel())
        member_with_max_level = player;

    uint32 gray_level = MaNGOS::XP::GetGrayLevel(player->getLevel());
    if( victim->getLevel() > gray_level && (!not_gray_member_with_max_level
        || not_gray_member_with_max_level->getLevel() < player->getLevel()))
        not_gray_member_with_max_level = player;
}

void Group::GetDataForXPAtKill(Unit const* victim, uint32& count,uint32& sum_level, Player* & member_with_max_level, Player* & not_gray_member_with_max_level, Player* additional)
{
    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* member = itr->getSource();
        if (!member || !member->isAlive())                  // only for alive
            continue;

        // will proccesed later
        if (member == additional)
            continue;

        if (!member->IsAtGroupRewardDistance(victim))       // at req. distance
            continue;

        ++count;
        GetDataForXPAtKill_helper(member,victim,sum_level,member_with_max_level,not_gray_member_with_max_level);
    }

    if (additional)
    {
        if (additional->IsAtGroupRewardDistance(victim))    // at req. distance
        {
            ++count;
            GetDataForXPAtKill_helper(additional,victim,sum_level,member_with_max_level,not_gray_member_with_max_level);
        }
    }
}

void Group::SendTargetIconList(WorldSession *session)
{
    if(!session)
        return;

    WorldPacket data(MSG_RAID_TARGET_UPDATE, (1+TARGET_ICON_COUNT*9));
    data << uint8(1);                                       // list targets

    for(int i = 0; i < TARGET_ICON_COUNT; ++i)
    {
        if (!m_targetIcons[i])
            continue;

        data << uint8(i);
        data << m_targetIcons[i];
    }

    session->SendPacket(&data);
}

void Group::SendUpdate()
{
    for(member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        Player* player = sObjectMgr.GetPlayer(citr->guid);
        if(!player || !player->GetSession() || player->GetGroup() != this )
            continue;
                                                            // guess size
        WorldPacket data(SMSG_GROUP_LIST, (1+1+1+1+8+4+GetMembersCount()*20));
        data << uint8(m_groupType);                         // group type (flags in 3.3)
        data << uint8(citr->group);                         // groupid
        data << uint8(GetFlags(*citr));                     // group flags
        data << uint8(isBGGroup() ? 1 : 0);                 // 2.0.x, isBattleGroundGroup?
        if(m_groupType & GROUPTYPE_LFD)
        {
            data << uint8(0);
            data << uint32(0);
        }
        data << GetObjectGuid();                            // group guid
        data << uint32(0);                                  // 3.3, this value increments every time SMSG_GROUP_LIST is sent
        data << uint32(GetMembersCount()-1);
        for(member_citerator citr2 = m_memberSlots.begin(); citr2 != m_memberSlots.end(); ++citr2)
        {
            if(citr->guid == citr2->guid)
                continue;
            Player* member = sObjectMgr.GetPlayer(citr2->guid);
            uint8 onlineState = (member) ? MEMBER_STATUS_ONLINE : MEMBER_STATUS_OFFLINE;
            onlineState = onlineState | ((isBGGroup()) ? MEMBER_STATUS_PVP : 0);

            data << citr2->name;
            data << citr2->guid;
            data << uint8(onlineState);                     // online-state
            data << uint8(citr2->group);                    // groupid
            data << uint8(GetFlags(*citr2));                // group flags
            data << uint8(0);                               // 3.3, role?
        }

        data << m_leaderGuid;                               // leader guid
        if(GetMembersCount()-1)
        {
            data << uint8(m_lootMethod);                    // loot method
            data << m_looterGuid;                           // looter guid
            data << uint8(m_lootThreshold);                 // loot threshold
            data << uint8(m_dungeonDifficulty);             // Dungeon Difficulty
            data << uint8(m_raidDifficulty);                // Raid Difficulty
            data << uint8(0);                               // 3.3, dynamic difficulty?
        }
        player->GetSession()->SendPacket( &data );
    }
}

void Group::UpdatePlayerOutOfRange(Player* pPlayer)
{
    if(!pPlayer || !pPlayer->IsInWorld())
        return;

    if (pPlayer->GetGroupUpdateFlag() == GROUP_UPDATE_FLAG_NONE)
        return;

    WorldPacket data;
    pPlayer->GetSession()->BuildPartyMemberStatsChangedPacket(pPlayer, &data);

    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
        if (Player *player = itr->getSource())
            if (player != pPlayer && !player->HaveAtClient(pPlayer))
                player->GetSession()->SendPacket(&data);
}

void Group::BroadcastPacket(WorldPacket *packet, bool ignorePlayersInBGRaid, int group, ObjectGuid ignore)
{
    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *pl = itr->getSource();
        if (!pl || (ignore && pl->GetObjectGuid() == ignore) || (ignorePlayersInBGRaid && pl->GetGroup() != this) )
            continue;

        if (pl->GetSession() && (group == -1 || itr->getSubGroup() == group))
            pl->GetSession()->SendPacket(packet);
    }
}

void Group::BroadcastReadyCheck(WorldPacket *packet)
{
    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *pl = itr->getSource();
        if (pl && pl->GetSession())
            if (IsLeader(pl->GetObjectGuid()) || IsAssistant(pl->GetObjectGuid()))
                pl->GetSession()->SendPacket(packet);
    }
}

void Group::OfflineReadyCheck()
{
    for(member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        Player *pl = sObjectMgr.GetPlayer(citr->guid);
        if (!pl || !pl->GetSession())
        {
            WorldPacket data(MSG_RAID_READY_CHECK_CONFIRM, 9);
            data << citr->guid;
            data << uint8(0);
            BroadcastReadyCheck(&data);
        }
    }
}

bool Group::_addMember(ObjectGuid guid, const char* name, bool isAssistant)
{
    // get first not-full group
    uint8 groupid = 0;
    if (m_subGroupsCounts)
    {
        bool groupFound = false;
        for (; groupid < MAX_RAID_SUBGROUPS; ++groupid)
        {
            if (m_subGroupsCounts[groupid] < MAX_GROUP_SIZE)
            {
                groupFound = true;
                break;
            }
        }
        // We are raid group and no one slot is free
        if (!groupFound)
            return false;
    }

    return _addMember(guid, name, isAssistant, groupid);
}

bool Group::_addMember(ObjectGuid guid, const char* name, bool isAssistant, uint8 group)
{
    if (IsFull())
        return false;

    if (!guid)
        return false;

    Player* player = sObjectMgr.GetPlayer(guid, false);

    uint32 lastMap = 0;
    if (player && player->IsInWorld())
        lastMap = player->GetMapId();
    else if (player && player->IsBeingTeleported())
        lastMap = player->GetTeleportDest().mapid;

    MemberSlot member;
    member.guid      = guid;
    member.name      = name;
    member.group     = group;
    member.assistant = isAssistant;
    member.lastMap   = lastMap;
    m_memberSlots.push_back(member);

    SubGroupCounterIncrease(group);

    if (player)
    {
        player->SetGroupInvite(NULL);
        //if player is in group and he is being added to BG raid group, then call SetBattleGroundRaid()
        if (player->GetGroup() && isBGGroup())
            player->SetBattleGroundRaid(this, group);
        //if player is in bg raid and we are adding him to normal group, then call SetOriginalGroup()
        else if (player->GetGroup())
            player->SetOriginalGroup(this, group);
        //if player is not in group, then call set group
        else
            player->SetGroup(this, group);

        if (player->IsInWorld())
        {
            // if the same group invites the player back, cancel the homebind timer
            if (InstanceGroupBind* bind = GetBoundInstance(player->GetMapId(), player))
                if (bind->state->GetInstanceId() == player->GetInstanceId())
                    player->m_InstanceValid = true;
        }
    }

    if (!isRaidGroup())                                     // reset targetIcons for non-raid-groups
    {
        for(int i = 0; i < TARGET_ICON_COUNT; ++i)
            m_targetIcons[i].Clear();
    }

    if (!isBGGroup())
    {
        // insert into group table
        CharacterDatabase.PExecute("INSERT INTO group_member(groupId,memberGuid,assistant,subgroup) VALUES('%u','%u','%u','%u')",
            m_Id, member.guid.GetCounter(), ((member.assistant==1)?1:0), member.group);
    }

    return true;
}

bool Group::_removeMember(ObjectGuid guid)
{
    Player *player = sObjectMgr.GetPlayer(guid);
    if (player)
    {
        //if we are removing player from battleground raid
        if (isBGGroup())
            player->RemoveFromBattleGroundRaid();
        else
        {
            //we can remove player who is in battleground from his original group
            if (player->GetOriginalGroup() == this)
                player->SetOriginalGroup(NULL);
            else
                player->SetGroup(NULL);
        }
    }

    _removeRolls(guid);

    member_witerator slot = _getMemberWSlot(guid);
    if (slot != m_memberSlots.end())
    {
        SubGroupCounterDecrease(slot->group);

        m_memberSlots.erase(slot);
    }

    if (!isBGGroup())
        CharacterDatabase.PExecute("DELETE FROM group_member WHERE memberGuid='%u'", guid.GetCounter());

    if (m_leaderGuid == guid)                               // leader was removed
    {
        if (GetMembersCount() > 0)
            _setLeader(m_memberSlots.front().guid);
        return true;
    }

    return false;
}

void Group::_setLeader(ObjectGuid guid)
{
    member_citerator slot = _getMemberCSlot(guid);
    if (slot == m_memberSlots.end())
        return;

    if (!isBGGroup())
    {
        uint32 slot_lowguid = slot->guid.GetCounter();

        uint32 leader_lowguid = m_leaderGuid.GetCounter();

        // TODO: set a time limit to have this function run rarely cause it can be slow
        CharacterDatabase.BeginTransaction();

        // update the group's bound instances when changing leaders

        // remove all permanent binds from the group
        // in the DB also remove solo binds that will be replaced with permbinds
        // from the new leader
        CharacterDatabase.PExecute(
            "DELETE FROM group_instance WHERE leaderguid='%u' AND (permanent = 1 OR "
            "instance IN (SELECT instance FROM character_instance WHERE guid = '%u')"
            ")", leader_lowguid, slot_lowguid);

        Player *player = sObjectMgr.GetPlayer(slot->guid);

        if (player)
        {
            for(uint8 i = 0; i < MAX_DIFFICULTY; ++i)
            {
                for(BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end();)
                {
                    if(itr->second.perm)
                    {
                        itr->second.state->RemoveGroup(this);
                        m_boundInstances[i].erase(itr++);
                    }
                    else
                        ++itr;
                }
            }
        }

        // update the group's solo binds to the new leader
        CharacterDatabase.PExecute("UPDATE group_instance SET leaderGuid='%u' WHERE leaderGuid = '%u'",
            slot_lowguid, leader_lowguid);

        // copy the permanent binds from the new leader to the group
        // overwriting the solo binds with permanent ones if necessary
        // in the DB those have been deleted already
        Player::ConvertInstancesToGroup(player, this, slot->guid);

        // update the group leader
        CharacterDatabase.PExecute("UPDATE groups SET leaderGuid='%u' WHERE groupId='%u'", slot_lowguid, m_Id);
        CharacterDatabase.CommitTransaction();
    }

    m_leaderGuid = slot->guid;
    m_leaderName = slot->name;
}

void Group::_removeRolls(ObjectGuid guid)
{
    for (Rolls::iterator it = RollId.begin(); it != RollId.end(); )
    {
        Roll* roll = *it;
        Roll::PlayerVote::iterator itr2 = roll->playerVote.find(guid);
        if(itr2 == roll->playerVote.end())
        {
            ++it;
            continue;
        }

        if (itr2->second == ROLL_GREED || itr2->second == ROLL_DISENCHANT)
            --roll->totalGreed;
        if (itr2->second == ROLL_NEED)
            --roll->totalNeed;
        if (itr2->second == ROLL_PASS)
            --roll->totalPass;
        if (itr2->second != ROLL_NOT_VALID)
            --roll->totalPlayersRolling;

        roll->playerVote.erase(itr2);

        if (!CountRollVote(guid, it, ROLL_NOT_EMITED_YET))
            ++it;
    }
}

bool Group::_setMembersGroup(ObjectGuid guid, uint8 group)
{
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return false;

    slot->group = group;

    SubGroupCounterIncrease(group);

    if (!isBGGroup())
        CharacterDatabase.PExecute("UPDATE group_member SET subgroup='%u' WHERE memberGuid='%u'", group, guid.GetCounter());

    return true;
}

bool Group::_setAssistantFlag(ObjectGuid guid, const bool &state)
{
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return false;

    slot->assistant = state;
    if (!isBGGroup())
        CharacterDatabase.PExecute("UPDATE group_member SET assistant='%u' WHERE memberGuid='%u'", (state==true)?1:0, guid.GetCounter());
    return true;
}

bool Group::_setMainTank(ObjectGuid guid)
{
    if (m_mainTankGuid == guid)
        return false;

    if (guid)
    {
        member_citerator slot = _getMemberCSlot(guid);
        if (slot == m_memberSlots.end())
            return false;

        if (m_mainAssistantGuid == guid)
            _setMainAssistant(ObjectGuid());
    }

    m_mainTankGuid = guid;

    if (!isBGGroup())
        CharacterDatabase.PExecute("UPDATE groups SET mainTank='%u' WHERE groupId='%u'", m_mainTankGuid.GetCounter(), m_Id);

    return true;
}

bool Group::_setMainAssistant(ObjectGuid guid)
{
    if (m_mainAssistantGuid == guid)
        return false;

    if (guid)
    {
        member_witerator slot = _getMemberWSlot(guid);
        if (slot == m_memberSlots.end())
            return false;

        if (m_mainTankGuid == guid)
            _setMainTank(ObjectGuid());
    }

    m_mainAssistantGuid = guid;

    if (!isBGGroup())
        CharacterDatabase.PExecute("UPDATE groups SET mainAssistant='%u' WHERE groupId='%u'",
            m_mainAssistantGuid.GetCounter(), m_Id);

    return true;
}

bool Group::SameSubGroup(Player const* member1, Player const* member2) const
{
    if(!member1 || !member2)
        return false;
    if (member1->GetGroup() != this || member2->GetGroup() != this)
        return false;
    else
        return member1->GetSubGroup() == member2->GetSubGroup();
}

// allows setting subgroup for offline members
void Group::ChangeMembersGroup(ObjectGuid guid, uint8 group)
{
    if (!isRaidGroup())
        return;

    Player *player = sObjectMgr.GetPlayer(guid);

    if (!player)
    {
        uint8 prevSubGroup = GetMemberGroup(guid);
        if (prevSubGroup == group)
            return;

        if (_setMembersGroup(guid, group))
        {
            SubGroupCounterDecrease(prevSubGroup);
            SendUpdate();
        }
    }
    else
        // This methods handles itself groupcounter decrease
        ChangeMembersGroup(player, group);
}

// only for online members
void Group::ChangeMembersGroup(Player *player, uint8 group)
{
    if (!player || !isRaidGroup())
        return;

    uint8 prevSubGroup = player->GetSubGroup();
    if (prevSubGroup == group)
        return;

    if (_setMembersGroup(player->GetObjectGuid(), group))
    {
        if (player->GetGroup() == this)
            player->GetGroupRef().setSubGroup(group);
        //if player is in BG raid, it is possible that he is also in normal raid - and that normal raid is stored in m_originalGroup reference
        else
        {
            prevSubGroup = player->GetOriginalSubGroup();
            player->GetOriginalGroupRef().setSubGroup(group);
        }
        SubGroupCounterDecrease(prevSubGroup);

        SendUpdate();
    }
}

uint32 Group::GetMaxSkillValueForGroup( SkillType skill )
{
    uint32 maxvalue = 0;

    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *member = itr->getSource();
        if (!member)
            continue;

        uint32 value = member->GetSkillValue(skill);
        if (maxvalue < value)
            maxvalue = value;
    }

    return maxvalue;
}

void Group::UpdateLooterGuid(WorldObject* pSource, bool ifneed)
{
    switch (GetLootMethod())
    {
        case MASTER_LOOT:
        case FREE_FOR_ALL:
            return;
        default:
            // round robin style looting applies for all low
            // quality items in each loot method except free for all and master loot
            break;
    }

    member_citerator guid_itr = _getMemberCSlot(GetLooterGuid());
    if (guid_itr != m_memberSlots.end())
    {
        if (ifneed)
        {
            // not update if only update if need and ok
            Player* looter = ObjectAccessor::FindPlayer(guid_itr->guid);
            if (looter && looter->IsWithinDist(pSource, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE), false))
                return;
        }
        ++guid_itr;
    }

    // search next after current
    if (guid_itr != m_memberSlots.end())
    {
        for(member_citerator itr = guid_itr; itr != m_memberSlots.end(); ++itr)
        {
            if (Player* pl = ObjectAccessor::FindPlayer(itr->guid))
            {
                if (pl->IsWithinDist(pSource, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE), false))
                {
                    bool refresh = pl->GetLootGuid() == pSource->GetObjectGuid();

                    //if(refresh)                           // update loot for new looter
                    //    pl->GetSession()->DoLootRelease(pl->GetLootGUID());
                    SetLooterGuid(pl->GetObjectGuid());
                    SendUpdate();
                    if (refresh)                            // update loot for new looter
                        pl->SendLoot(pSource->GetObjectGuid(), LOOT_CORPSE);
                    return;
                }
            }
        }
    }

    // search from start
    for(member_citerator itr = m_memberSlots.begin(); itr != guid_itr; ++itr)
    {
        if (Player* pl = ObjectAccessor::FindPlayer(itr->guid))
        {
            if (pl->IsWithinDist(pSource, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE), false))
            {
                bool refresh = pl->GetLootGuid() == pSource->GetObjectGuid();

                //if(refresh)                               // update loot for new looter
                //    pl->GetSession()->DoLootRelease(pl->GetLootGUID());
                SetLooterGuid(pl->GetObjectGuid());
                SendUpdate();
                if (refresh)                                // update loot for new looter
                    pl->SendLoot(pSource->GetObjectGuid(), LOOT_CORPSE);
                return;
            }
        }
    }

    SetLooterGuid(ObjectGuid());
    SendUpdate();
}

GroupJoinBattlegroundResult Group::CanJoinBattleGroundQueue(BattleGround const* bgOrTemplate, BattleGroundQueueTypeId bgQueueTypeId, uint32 MinPlayerCount, uint32 MaxPlayerCount, bool isRated, uint32 arenaSlot)
{
    BattlemasterListEntry const* bgEntry = sBattlemasterListStore.LookupEntry(bgOrTemplate->GetTypeID());
    if(!bgEntry)
        return ERR_GROUP_JOIN_BATTLEGROUND_FAIL;            // shouldn't happen

    // check for min / max count
    uint32 memberscount = GetMembersCount();

    // only check for MinPlayerCount since MinPlayerCount == MaxPlayerCount for arenas...
    if(bgOrTemplate->isArena() && memberscount != MinPlayerCount)
        return ERR_ARENA_TEAM_PARTY_SIZE;

    if(memberscount > bgEntry->maxGroupSize)                // no MinPlayerCount for battlegrounds
        return ERR_BATTLEGROUND_NONE;                       // ERR_GROUP_JOIN_BATTLEGROUND_TOO_MANY handled on client side

    // get a player as reference, to compare other players' stats to (arena team id, queue id based on level, etc.)
    Player * reference = GetFirstMember()->getSource();
    // no reference found, can't join this way
    if(!reference)
        return ERR_BATTLEGROUND_JOIN_FAILED;

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgOrTemplate->GetMapId(), reference->getLevel());
    if(!bracketEntry)
        return ERR_BATTLEGROUND_JOIN_FAILED;

    uint32 arenaTeamId = reference->GetArenaTeamId(arenaSlot);
    Team team = reference->GetTeam();

    // check every member of the group to be able to join
    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *member = itr->getSource();
        // offline member? don't let join
        if(!member)
            return ERR_BATTLEGROUND_JOIN_FAILED;
        // don't allow cross-faction join as group
        if(member->GetTeam() != team)
            return ERR_BATTLEGROUND_JOIN_TIMED_OUT;
        // not in the same battleground level bracket, don't let join
        PvPDifficultyEntry const* memberBracketEntry = GetBattlegroundBracketByLevel(bracketEntry->mapId, member->getLevel());
        if(memberBracketEntry != bracketEntry)
            return ERR_BATTLEGROUND_JOIN_RANGE_INDEX;
        // don't let join rated matches if the arena team id doesn't match
        if(isRated && member->GetArenaTeamId(arenaSlot) != arenaTeamId)
            return ERR_BATTLEGROUND_JOIN_FAILED;
        // don't let join if someone from the group is already in that bg queue
        if(member->InBattleGroundQueueForBattleGroundQueueType(bgQueueTypeId))
            return ERR_BATTLEGROUND_JOIN_FAILED;            // not blizz-like
        // check for deserter debuff in case not arena queue
        if(bgOrTemplate->GetTypeID() != BATTLEGROUND_AA && !member->CanJoinToBattleground())
            return ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS;
        // check if member can join any more battleground queues
        if(!member->HasFreeBattleGroundQueueId())
            return ERR_BATTLEGROUND_TOO_MANY_QUEUES;        // not blizz-like
    }
    return GroupJoinBattlegroundResult(bgOrTemplate->GetTypeID());
}

void Group::SetDungeonDifficulty(Difficulty difficulty)
{
    m_dungeonDifficulty = difficulty;
    if(!isBGGroup())
        CharacterDatabase.PExecute("UPDATE groups SET difficulty = %u WHERE groupId='%u'", m_dungeonDifficulty, m_Id);

    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *player = itr->getSource();
        if(!player->GetSession() || player->getLevel() < LEVELREQUIREMENT_HEROIC)
            continue;
        player->SetDungeonDifficulty(difficulty);
        player->SendDungeonDifficulty(true);
    }
}

void Group::SetRaidDifficulty(Difficulty difficulty)
{
    m_raidDifficulty = difficulty;
    if(!isBGGroup())
        CharacterDatabase.PExecute("UPDATE groups SET raiddifficulty = %u WHERE groupId='%u'", m_raidDifficulty, m_Id);

    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *player = itr->getSource();
        if(!player->GetSession() || player->getLevel() < LEVELREQUIREMENT_HEROIC)
            continue;
        player->SetRaidDifficulty(difficulty);
        player->SendRaidDifficulty(true);
    }
}

bool Group::InCombatToInstance(uint32 instanceId)
{
    for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *pPlayer = itr->getSource();
        if(pPlayer->getAttackers().size() && pPlayer->GetInstanceId() == instanceId)
            return true;
    }
    return false;
}

bool Group::SetPlayerMap(ObjectGuid guid, uint32 mapid)
{
    member_witerator slot = _getMemberWSlot(guid);
    if (slot != m_memberSlots.end())
    {
        slot->lastMap = mapid;
        DEBUG_LOG("Group::SetPlayerMap> map is updated");
        return true;
    }
    return false;
}

void Group::ResetInstances(InstanceResetMethod method, bool isRaid, Player* SendMsgTo)
{
    if (isBGGroup())
        return;

    // method can be INSTANCE_RESET_ALL, INSTANCE_RESET_CHANGE_DIFFICULTY, INSTANCE_RESET_GROUP_DISBAND

    // we assume that when the difficulty changes, all instances that can be reset will be
    Difficulty diff = GetDifficulty(isRaid);

    typedef std::set<uint32> OfflineMapSet;
    OfflineMapSet mapsWithOfflinePlayer;                    // to store map of offline players

    if (method != INSTANCE_RESET_GROUP_DISBAND)
    {
        // Store maps in which are offline members for instance reset check.
        for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        {
            if (!ObjectAccessor::FindPlayer(itr->guid))
                mapsWithOfflinePlayer.insert(itr->lastMap); // add last map from offline player
        }
    }

    for (BoundInstancesMap::iterator itr = m_boundInstances[diff].begin(); itr != m_boundInstances[diff].end();)
    {
        DungeonPersistentState* state = itr->second.state;
        const MapEntry* entry = sMapStore.LookupEntry(itr->first);
        if (!entry || entry->IsRaid() != isRaid || (!state->CanReset() && method != INSTANCE_RESET_GROUP_DISBAND))
        {
            ++itr;
            continue;
        }

        if (method == INSTANCE_RESET_ALL)
        {
            // the "reset all instances" method can only reset normal maps
            if (entry->map_type == MAP_RAID || diff == DUNGEON_DIFFICULTY_HEROIC)
            {
                ++itr;
                continue;
            }
        }

        bool isEmpty = true;
        // check if there are offline members on the map
        if (method != INSTANCE_RESET_GROUP_DISBAND && mapsWithOfflinePlayer.find(state->GetMapId()) != mapsWithOfflinePlayer.end())
            isEmpty = false;

        // if the map is loaded, reset it if can
        if (isEmpty && entry->IsDungeon() && !(method == INSTANCE_RESET_GROUP_DISBAND && !state->CanReset()))
            if (Map* map = sMapMgr.FindMap(state->GetMapId(), state->GetInstanceId()))
                isEmpty = ((DungeonMap*)map)->Reset(method);

        if (SendMsgTo)
        {
            if (isEmpty)
                SendMsgTo->SendResetInstanceSuccess(state->GetMapId());
            else
                SendMsgTo->SendResetInstanceFailed(0, state->GetMapId());
        }

        // TODO - Adapt here when clear how difficulty changes must be handled
        if (isEmpty || method == INSTANCE_RESET_GROUP_DISBAND || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
        {
            // do not reset the instance, just unbind if others are permanently bound to it
            if (state->CanReset())
                state->DeleteFromDB();
            else
                CharacterDatabase.PExecute("DELETE FROM group_instance WHERE instance = '%u'", state->GetInstanceId());
            // i don't know for sure if hash_map iterators
            m_boundInstances[diff].erase(itr);
            itr = m_boundInstances[diff].begin();
            // this unloads the instance save unless online players are bound to it
            // (eg. permanent binds or GM solo binds)
            state->RemoveGroup(this);
        }
        else
            ++itr;
    }
}

InstanceGroupBind* Group::GetBoundInstance(uint32 mapid, Player* player)
{
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapid);
    if (!mapEntry)
        return NULL;

    Difficulty difficulty = player->GetDifficulty(mapEntry->IsRaid());

    // some instances only have one difficulty
    MapDifficulty const* mapDiff = GetMapDifficultyData(mapid,difficulty);
    if (!mapDiff)
        difficulty = DUNGEON_DIFFICULTY_NORMAL;

    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    if (itr != m_boundInstances[difficulty].end())
        return &itr->second;
    else
        return NULL;
}

InstanceGroupBind* Group::GetBoundInstance(Map* aMap, Difficulty difficulty)
{
    // some instances only have one difficulty
    MapDifficulty const* mapDiff = GetMapDifficultyData(aMap->GetId(),difficulty);
    if (!mapDiff)
        return NULL;

    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(aMap->GetId());
    if (itr != m_boundInstances[difficulty].end())
        return &itr->second;
    else
        return NULL;
}

InstanceGroupBind* Group::BindToInstance(DungeonPersistentState *state, bool permanent, bool load)
{
    if (state && !isBGGroup())
    {
        InstanceGroupBind& bind = m_boundInstances[state->GetDifficulty()][state->GetMapId()];
        if (bind.state)
        {
            // when a boss is killed or when copying the players's binds to the group
            if (permanent != bind.perm || state != bind.state)
                if (!load)
                    CharacterDatabase.PExecute("UPDATE group_instance SET instance = '%u', permanent = '%u' WHERE leaderGuid = '%u' AND instance = '%u'",
                        state->GetInstanceId(), permanent, GetLeaderGuid().GetCounter(), bind.state->GetInstanceId());
        }
        else if (!load)
            CharacterDatabase.PExecute("INSERT INTO group_instance (leaderGuid, instance, permanent) VALUES ('%u', '%u', '%u')",
                GetLeaderGuid().GetCounter(), state->GetInstanceId(), permanent);

        if (bind.state != state)
        {
            if (bind.state)
                bind.state->RemoveGroup(this);
            state->AddGroup(this);
        }

        bind.state = state;
        bind.perm = permanent;
        if (!load)
            DEBUG_LOG("Group::BindToInstance: Group (Id: %d) is now bound to map %d, instance %d, difficulty %d",
                GetId(), state->GetMapId(), state->GetInstanceId(), state->GetDifficulty());
        return &bind;
    }
    else
        return NULL;
}

void Group::UnbindInstance(uint32 mapid, uint8 difficulty, bool unload)
{
    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    if (itr != m_boundInstances[difficulty].end())
    {
        if (!unload)
            CharacterDatabase.PExecute("DELETE FROM group_instance WHERE leaderGuid = '%u' AND instance = '%u'",
                GetLeaderGuid().GetCounter(), itr->second.state->GetInstanceId());
        itr->second.state->RemoveGroup(this);                // state can become invalid
        m_boundInstances[difficulty].erase(itr);
    }
}

void Group::_homebindIfInstance(Player *player)
{
    if (player && !player->isGameMaster())
    {
        Map* map = player->GetMap();
        if (map->IsDungeon())
        {
            // leaving the group in an instance, the homebind timer is started
            // unless the player is permanently saved to the instance
            InstancePlayerBind *playerBind = player->GetBoundInstance(map->GetId(), map->GetDifficulty());
            if(!playerBind || !playerBind->perm)
                player->m_InstanceValid = false;
        }
    }
}

static void RewardGroupAtKill_helper(Player* pGroupGuy, Unit* pVictim, uint32 count, bool PvP, float group_rate, uint32 sum_level, bool is_dungeon, Player* not_gray_member_with_max_level, Player* member_with_max_level, uint32 xp )
{
    // honor can be in PvP and !PvP (racial leader) cases (for alive)
    if (pGroupGuy->isAlive())
        pGroupGuy->RewardHonor(pVictim,count);

    // xp and reputation only in !PvP case
    if(!PvP)
    {
        float rate = group_rate * float(pGroupGuy->getLevel()) / sum_level;

        // if is in dungeon then all receive full reputation at kill
        // rewarded any alive/dead/near_corpse group member
        pGroupGuy->RewardReputation(pVictim,is_dungeon ? 1.0f : rate);

        // XP updated only for alive group member
        if(pGroupGuy->isAlive() && not_gray_member_with_max_level &&
            pGroupGuy->getLevel() <= not_gray_member_with_max_level->getLevel())
        {
            uint32 itr_xp = (member_with_max_level == not_gray_member_with_max_level) ? uint32(xp*rate) : uint32((xp*rate/2)+1);

            pGroupGuy->GiveXP(itr_xp, pVictim);
            if(Pet* pet = pGroupGuy->GetPet())
                pet->GivePetXP(itr_xp/2);
        }

        // quest objectives updated only for alive group member or dead but with not released body
        if(pGroupGuy->isAlive()|| !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        {
            // normal creature (not pet/etc) can be only in !PvP case
            if(pVictim->GetTypeId()==TYPEID_UNIT)
                if(CreatureInfo const* normalInfo = ObjectMgr::GetCreatureTemplate(pVictim->GetEntry()))
                    pGroupGuy->KilledMonster(normalInfo, pVictim->GetObjectGuid());
        }
    }
}

/** Provide rewards to group members at unit kill
 *
 * @param pVictim       Killed unit
 * @param player_tap    Player who tap unit if online, it can be group member or can be not if leaved after tap but before kill target
 *
 * Rewards received by group members and player_tap
 */
void Group::RewardGroupAtKill(Unit* pVictim, Player* player_tap)
{
    bool PvP = pVictim->isCharmedOwnedByPlayerOrPlayer();

    // prepare data for near group iteration (PvP and !PvP cases)
    uint32 xp = 0;

    uint32 count = 0;
    uint32 sum_level = 0;
    Player* member_with_max_level = NULL;
    Player* not_gray_member_with_max_level = NULL;

    GetDataForXPAtKill(pVictim,count,sum_level,member_with_max_level,not_gray_member_with_max_level,player_tap);

    if(member_with_max_level)
    {
        /// not get Xp in PvP or no not gray players in group
        xp = (PvP || !not_gray_member_with_max_level) ? 0 : MaNGOS::XP::Gain(not_gray_member_with_max_level, pVictim);

        /// skip in check PvP case (for speed, not used)
        bool is_raid = PvP ? false : sMapStore.LookupEntry(pVictim->GetMapId())->IsRaid() && isRaidGroup();
        bool is_dungeon = PvP ? false : sMapStore.LookupEntry(pVictim->GetMapId())->IsDungeon();
        float group_rate = MaNGOS::XP::xp_in_group_rate(count,is_raid);

        for(GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if(!pGroupGuy)
                continue;

            // will proccessed later
            if(pGroupGuy==player_tap)
                continue;

            if(!pGroupGuy->IsAtGroupRewardDistance(pVictim))
                continue;                               // member (alive or dead) or his corpse at req. distance

            RewardGroupAtKill_helper(pGroupGuy, pVictim, count, PvP, group_rate, sum_level, is_dungeon, not_gray_member_with_max_level, member_with_max_level, xp);
        }

        if(player_tap)
        {
            // member (alive or dead) or his corpse at req. distance
            if(player_tap->IsAtGroupRewardDistance(pVictim))
                RewardGroupAtKill_helper(player_tap, pVictim, count, PvP, group_rate, sum_level, is_dungeon, not_gray_member_with_max_level, member_with_max_level, xp);
        }
    }
}
