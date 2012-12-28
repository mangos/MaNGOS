/*
 * Copyright (C) 2005-2013 MaNGOS <http://getmangos.com/>
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
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "Opcodes.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Database/DatabaseEnv.h"
#include "ChannelMgr.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Player.h"
#include "SpellAuras.h"
#include "Language.h"
#include "Util.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

bool WorldSession::processChatmessageFurtherAfterSecurityChecks(std::string& msg, uint32 lang)
{
    if (lang != LANG_ADDON)
    {
        // strip invisible characters for non-addon messages
        if (sWorld.getConfig(CONFIG_BOOL_CHAT_FAKE_MESSAGE_PREVENTING))
            stripLineInvisibleChars(msg);

        if (sWorld.getConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_SEVERITY) && GetSecurity() < SEC_MODERATOR
                && !ChatHandler(this).isValidChatMessage(msg.c_str()))
        {
            sLog.outError("Player %s (GUID: %u) sent a chatmessage with an invalid link: %s", GetPlayer()->GetName(),
                          GetPlayer()->GetGUIDLow(), msg.c_str());
            if (sWorld.getConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_KICK))
                KickPlayer();
            return false;
        }
    }

    return true;
}

void WorldSession::HandleMessagechatOpcode(WorldPacket& recv_data)
{
    uint32 type;
    uint32 lang;

    switch (recv_data.GetOpcode())
    {
        case CMSG_MESSAGECHAT_SAY:          type = CHAT_MSG_SAY;            break;
        case CMSG_MESSAGECHAT_YELL:         type = CHAT_MSG_YELL;           break;
        case CMSG_MESSAGECHAT_CHANNEL:      type = CHAT_MSG_CHANNEL;        break;
        case CMSG_MESSAGECHAT_WHISPER:      type = CHAT_MSG_WHISPER;        break;
        case CMSG_MESSAGECHAT_GUILD:        type = CHAT_MSG_GUILD;          break;
        case CMSG_MESSAGECHAT_OFFICER:      type = CHAT_MSG_OFFICER;        break;
        case CMSG_MESSAGECHAT_AFK:          type = CHAT_MSG_AFK;            break;
        case CMSG_MESSAGECHAT_DND:          type = CHAT_MSG_DND;            break;
        case CMSG_MESSAGECHAT_EMOTE:        type = CHAT_MSG_EMOTE;          break;
        case CMSG_MESSAGECHAT_PARTY:        type = CHAT_MSG_PARTY;          break;
        case CMSG_MESSAGECHAT_RAID:         type = CHAT_MSG_RAID;           break;
        case CMSG_MESSAGECHAT_BATTLEGROUND: type = CHAT_MSG_BATTLEGROUND;   break;
        case CMSG_MESSAGECHAT_RAID_WARNING: type = CHAT_MSG_RAID_WARNING;   break;
        default:
            sLog.outError("HandleMessagechatOpcode : Unknown chat opcode (0x%X)", recv_data.GetOpcode());
            recv_data.rfinish();
            return;
    }

    // no language sent with emote packet.
    if (type != CHAT_MSG_EMOTE && type != CHAT_MSG_AFK && type != CHAT_MSG_DND)
    {
        recv_data >> lang;

        // prevent talking at unknown language (cheating)
        LanguageDesc const* langDesc = GetLanguageDescByID(lang);
        if (!langDesc)
        {
            SendNotification(LANG_UNKNOWN_LANGUAGE);
            return;
        }
        if (langDesc->skill_id != 0 && !_player->HasSkill(langDesc->skill_id))
        {
            // also check SPELL_AURA_COMPREHEND_LANGUAGE (client offers option to speak in that language)
            Unit::AuraList const& langAuras = _player->GetAurasByType(SPELL_AURA_COMPREHEND_LANGUAGE);
            bool foundAura = false;
            for (Unit::AuraList::const_iterator i = langAuras.begin(); i != langAuras.end(); ++i)
            {
                if ((*i)->GetModifier()->m_miscvalue == int32(lang))
                {
                    foundAura = true;
                    break;
                }
            }
            if (!foundAura)
            {
                SendNotification(LANG_NOT_LEARNED_LANGUAGE);
                return;
            }
        }

        if (lang == LANG_ADDON)
        {
            // Disabled addon channel?
            if (!sWorld.getConfig(CONFIG_BOOL_ADDON_CHANNEL))
                return;
        }
        // LANG_ADDON should not be changed nor be affected by flood control
        else
        {
            // send in universal language if player in .gmon mode (ignore spell effects)
            if (_player->isGameMaster())
                lang = LANG_UNIVERSAL;
            else
            {
                // send in universal language in two side iteration allowed mode
                if (sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT))
                    lang = LANG_UNIVERSAL;
                else
                {
                    switch (type)
                    {
                        case CHAT_MSG_PARTY:
                        case CHAT_MSG_PARTY_LEADER:
                        case CHAT_MSG_RAID:
                        case CHAT_MSG_RAID_LEADER:
                        case CHAT_MSG_RAID_WARNING:
                            // allow two side chat at group channel if two side group allowed
                            if (sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GROUP))
                                lang = LANG_UNIVERSAL;
                            break;
                        case CHAT_MSG_GUILD:
                        case CHAT_MSG_OFFICER:
                            // allow two side chat at guild channel if two side guild allowed
                            if (sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GUILD))
                                lang = LANG_UNIVERSAL;
                            break;
                    }
                }

                // but overwrite it by SPELL_AURA_MOD_LANGUAGE auras (only single case used)
                Unit::AuraList const& ModLangAuras = _player->GetAurasByType(SPELL_AURA_MOD_LANGUAGE);
                if (!ModLangAuras.empty())
                    lang = ModLangAuras.front()->GetModifier()->m_miscvalue;
            }

            if (type != CHAT_MSG_AFK && type != CHAT_MSG_DND)
            {
                if (!_player->CanSpeak())
                {
                    std::string timeStr = secsToTimeString(m_muteTime - time(NULL));
                    SendNotification(GetMangosString(LANG_WAIT_BEFORE_SPEAKING), timeStr.c_str());
                    return;
                }

                GetPlayer()->UpdateSpeakTime();
            }
        }
    }
    else
        lang = LANG_UNIVERSAL;

    DEBUG_LOG("CHAT: packet received. type %u lang %u", type, lang);

    switch (type)
    {
        case CHAT_MSG_SAY:
        case CHAT_MSG_EMOTE:
        case CHAT_MSG_YELL:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()))
                break;

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            if (type == CHAT_MSG_SAY)
                GetPlayer()->Say(msg, lang);
            else if (type == CHAT_MSG_EMOTE)
                GetPlayer()->TextEmote(msg);
            else if (type == CHAT_MSG_YELL)
                GetPlayer()->Yell(msg, lang);
        } break;

        case CHAT_MSG_WHISPER:
        {
            std::string to, msg;
            uint32 toLength = recv_data.ReadBits(10);
            uint32 msgLength = recv_data.ReadBits(9);
            to = recv_data.ReadString(toLength);
            msg = recv_data.ReadString(msgLength);

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            if (!normalizePlayerName(to))
            {
                SendPlayerNotFoundNotice(to);
                break;
            }

            Player* player = sObjectMgr.GetPlayer(to.c_str());
            uint32 tSecurity = GetSecurity();
            uint32 pSecurity = player ? player->GetSession()->GetSecurity() : SEC_PLAYER;
            if (!player || (tSecurity == SEC_PLAYER && pSecurity > SEC_PLAYER && !player->isAcceptWhispers()))
            {
                SendPlayerNotFoundNotice(to);
                return;
            }

            if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT) && tSecurity == SEC_PLAYER && pSecurity == SEC_PLAYER)
            {
                if (GetPlayer()->GetTeam() != player->GetTeam())
                {
                    SendWrongFactionNotice();
                    return;
                }
            }

            GetPlayer()->Whisper(msg, lang, player->GetObjectGuid());
        } break;

        case CHAT_MSG_PARTY:
        case CHAT_MSG_PARTY_LEADER:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()))
                break;

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            // if player is in battleground, he cannot say to battleground members by /p
            Group* group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = _player->GetGroup();
                if (!group || group->isBGGroup())
                    return;
            }

            if ((type == CHAT_MSG_PARTY_LEADER) && !group->IsLeader(_player->GetObjectGuid()))
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, type, lang, msg.c_str());
            group->BroadcastPacket(&data, false, group->GetMemberGroup(GetPlayer()->GetObjectGuid()));

            break;
        }
        case CHAT_MSG_GUILD:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()))
                break;

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            if (GetPlayer()->GetGuildId())
                if (Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId()))
                    guild->BroadcastToGuild(this, msg, lang == LANG_ADDON ? LANG_ADDON : LANG_UNIVERSAL);

            break;
        }
        case CHAT_MSG_OFFICER:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()))
                break;

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            if (GetPlayer()->GetGuildId())
                if (Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId()))
                    guild->BroadcastToOfficers(this, msg, lang == LANG_ADDON ? LANG_ADDON : LANG_UNIVERSAL);

            break;
        }
        case CHAT_MSG_RAID:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()))
                break;

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            // if player is in battleground, he cannot say to battleground members by /ra
            Group* group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = GetPlayer()->GetGroup();
                if (!group || group->isBGGroup() || !group->isRaidGroup())
                    return;
            }

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_RAID, lang, msg.c_str());
            group->BroadcastPacket(&data, false);
        } break;
        case CHAT_MSG_RAID_LEADER:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()))
                break;

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            // if player is in battleground, he cannot say to battleground members by /ra
            Group* group = GetPlayer()->GetOriginalGroup();
            if (!group)
            {
                group = GetPlayer()->GetGroup();
                if (!group || group->isBGGroup() || !group->isRaidGroup() || !group->IsLeader(_player->GetObjectGuid()))
                    return;
            }

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_RAID_LEADER, lang, msg.c_str());
            group->BroadcastPacket(&data, false);
        } break;

        case CHAT_MSG_RAID_WARNING:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            Group* group = GetPlayer()->GetGroup();
            if (!group || !group->isRaidGroup() ||
                    !(group->IsLeader(GetPlayer()->GetObjectGuid()) || group->IsAssistant(GetPlayer()->GetObjectGuid())))
                return;

            WorldPacket data;
            // in battleground, raid warning is sent only to players in battleground - code is ok
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_RAID_WARNING, lang, msg.c_str());
            group->BroadcastPacket(&data, false);
        } break;

        case CHAT_MSG_BATTLEGROUND:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            // battleground raid is always in Player->GetGroup(), never in GetOriginalGroup()
            Group* group = GetPlayer()->GetGroup();
            if (!group || !group->isBGGroup())
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_BATTLEGROUND, lang, msg.c_str());
            group->BroadcastPacket(&data, false);
        } break;

        case CHAT_MSG_BATTLEGROUND_LEADER:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            // battleground raid is always in Player->GetGroup(), never in GetOriginalGroup()
            Group* group = GetPlayer()->GetGroup();
            if (!group || !group->isBGGroup() || !group->IsLeader(GetPlayer()->GetObjectGuid()))
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_BATTLEGROUND_LEADER, lang, msg.c_str());
            group->BroadcastPacket(&data, false);
        } break;

        case CHAT_MSG_CHANNEL:
        {
            std::string channel, msg;
            uint32 channelLength = recv_data.ReadBits(10);
            uint32 msgLength = recv_data.ReadBits(9);
            msg = recv_data.ReadString(msgLength);
            channel = recv_data.ReadString(channelLength);

            if (!processChatmessageFurtherAfterSecurityChecks(msg, lang))
                return;

            if (msg.empty())
                break;

            if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
                if (Channel* chn = cMgr->GetChannel(channel, _player))
                    chn->Say(_player->GetObjectGuid(), msg.c_str(), lang);
        } break;

        case CHAT_MSG_AFK:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (!_player->isInCombat())
            {
                if (_player->isAFK())                       // Already AFK
                {
                    if (msg.empty())
                        _player->ToggleAFK();               // Remove AFK
                    else
                        _player->autoReplyMsg = msg;        // Update message
                }
                else                                        // New AFK mode
                {
                    _player->autoReplyMsg = msg.empty() ? GetMangosString(LANG_PLAYER_AFK_DEFAULT) : msg;

                    if (_player->isDND())
                        _player->ToggleDND();

                    _player->ToggleAFK();
                }
            }
            break;
        }
        case CHAT_MSG_DND:
        {
            std::string msg;
            msg = recv_data.ReadString(recv_data.ReadBits(9));

            if (_player->isDND())                           // Already DND
            {
                if (msg.empty())
                    _player->ToggleDND();                   // Remove DND
                else
                    _player->autoReplyMsg = msg;            // Update message
            }
            else                                            // New DND mode
            {
                _player->autoReplyMsg = msg.empty() ? GetMangosString(LANG_PLAYER_DND_DEFAULT) : msg;

                if (_player->isAFK())
                    _player->ToggleAFK();

                _player->ToggleDND();
            }
            break;
        }

        default:
            sLog.outError("CHAT: unknown message type %u, lang: %u", type, lang);
            break;
    }
}

void WorldSession::HandleAddonMessagechatOpcode(WorldPacket& recv_data)
{
    uint32 type;

    switch (recv_data.GetOpcode())
    {
        case CMSG_MESSAGECHAT_ADDON_BATTLEGROUND:   type = CHAT_MSG_BATTLEGROUND;   break;
        case CMSG_MESSAGECHAT_ADDON_GUILD:          type = CHAT_MSG_GUILD;          break;
        case CMSG_MESSAGECHAT_ADDON_OFFICER:        type = CHAT_MSG_OFFICER;        break;
        case CMSG_MESSAGECHAT_ADDON_PARTY:          type = CHAT_MSG_PARTY;          break;
        case CMSG_MESSAGECHAT_ADDON_RAID:           type = CHAT_MSG_RAID;           break;
        case CMSG_MESSAGECHAT_ADDON_WHISPER:        type = CHAT_MSG_WHISPER;        break;
        default:
            sLog.outError("HandleAddonMessagechatOpcode: Unknown addon chat opcode (0x%X)", recv_data.GetOpcode());
            recv_data.rfinish();
            return;
    }

    // Disabled addon channel?
    if (!sWorld.getConfig(CONFIG_BOOL_ADDON_CHANNEL))
        return;

    switch (type)
    {
        case CHAT_MSG_BATTLEGROUND:
        {
            uint32 msgLen = recv_data.ReadBits(9);
            uint32 prefixLen = recv_data.ReadBits(5);
            std::string msg = recv_data.ReadString(msgLen);
            std::string prefix = recv_data.ReadString(prefixLen);

            Group* group = _player->GetGroup();
            if (!group || !group->isBGGroup())
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, type, LANG_ADDON, "", ObjectGuid(), msg.c_str(), NULL);
            group->BroadcastPacket(&data, false);
            break;
        }
        case CHAT_MSG_GUILD:
        {
            uint32 msgLen = recv_data.ReadBits(9);
            uint32 prefixLen = recv_data.ReadBits(5);
            std::string msg = recv_data.ReadString(msgLen);
            std::string prefix = recv_data.ReadString(prefixLen);

            if (_player->GetGuildId())
                if (Guild* guild = sGuildMgr.GetGuildById(_player->GetGuildId()))
                    guild->BroadcastAddonToGuild(this, msg, prefix);

            break;
        }
        case CHAT_MSG_OFFICER:
        {
            uint32 prefixLen = recv_data.ReadBits(5);
            uint32 msgLen = recv_data.ReadBits(9);
            std::string prefix = recv_data.ReadString(prefixLen);
            std::string msg = recv_data.ReadString(msgLen);

            if (_player->GetGuildId())
                if (Guild* guild = sGuildMgr.GetGuildById(_player->GetGuildId()))
                    guild->BroadcastAddonToOfficers(this, msg, prefix);
            break;
        }
        case CHAT_MSG_WHISPER:
        {
            uint32 msgLen = recv_data.ReadBits(9);
            uint32 prefixLen = recv_data.ReadBits(5);
            uint32 targetLen = recv_data.ReadBits(10);
            std::string msg = recv_data.ReadString(msgLen);
            std::string prefix = recv_data.ReadString(prefixLen);
            std::string targetName = recv_data.ReadString(targetLen);

            if (!normalizePlayerName(targetName))
                break;

            Player* receiver = sObjectMgr.GetPlayer(targetName.c_str());
            if (!receiver)
                break;

            _player->WhisperAddon(msg, prefix, receiver->GetObjectGuid());
            break;
        }
        // Messages sent to "RAID" while in a party will get delivered to "PARTY"
        case CHAT_MSG_PARTY:
        case CHAT_MSG_RAID:
        {
            uint32 prefixLen = recv_data.ReadBits(5);
            uint32 msgLen = recv_data.ReadBits(9);
            std::string prefix = recv_data.ReadString(prefixLen);
            std::string msg = recv_data.ReadString(msgLen);

            Group* group = _player->GetGroup();
            if (!group || group->isBGGroup())
                break;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, type, LANG_ADDON, "", ObjectGuid(), msg.c_str(), NULL, prefix.c_str());
            group->BroadcastPacket(&data, false, group->GetMemberGroup(_player->GetObjectGuid()));
            break;
        }
        default:
        {
            sLog.outError("HandleAddonMessagechatOpcode: unknown addon message type %u", type);
            break;
        }
    }
}

void WorldSession::HandleEmoteOpcode(WorldPacket& recv_data)
{
    if (!GetPlayer()->isAlive() || GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        return;

    uint32 emote;
    recv_data >> emote;
    DEBUG_LOG("CMSG_EMOTE %u", emote);
    GetPlayer()->HandleEmoteCommand(emote);
}

namespace MaNGOS
{
    class EmoteChatBuilder
    {
        public:
            EmoteChatBuilder(Player const& pl, uint32 text_emote, uint32 emote_num, Unit const* target)
                : i_player(pl), i_text_emote(text_emote), i_emote_num(emote_num), i_target(target) {}

            void operator()(WorldPacket& data, int32 loc_idx)
            {
                char const* nam = i_target ? i_target->GetNameForLocaleIdx(loc_idx) : NULL;
                uint32 namlen = (nam ? strlen(nam) : 0) + 1;

                data.Initialize(SMSG_TEXT_EMOTE, (20 + namlen));
                data << ObjectGuid(i_player.GetObjectGuid());
                data << uint32(i_text_emote);
                data << uint32(i_emote_num);
                data << uint32(namlen);
                if (namlen > 1)
                    data.append(nam, namlen);
                else
                    data << uint8(0x00);

                DEBUG_LOG("SMSG_TEXT_EMOTE i_text_emote %u i_emote_num %u",
                    i_text_emote, i_emote_num);
            }

        private:
            Player const& i_player;
            uint32        i_text_emote;
            uint32        i_emote_num;
            Unit const*   i_target;
    };
}                                                           // namespace MaNGOS

void WorldSession::HandleTextEmoteOpcode(WorldPacket& recv_data)
{
    if (!GetPlayer()->isAlive())
        return;

    if (!GetPlayer()->CanSpeak())
    {
        std::string timeStr = secsToTimeString(m_muteTime - time(NULL));
        SendNotification(GetMangosString(LANG_WAIT_BEFORE_SPEAKING), timeStr.c_str());
        return;
    }

    uint32 text_emote, emoteNum;
    ObjectGuid guid;

    recv_data >> text_emote;
    recv_data >> emoteNum;
    recv_data >> guid;

    EmotesTextEntry const* em = sEmotesTextStore.LookupEntry(text_emote);
    if (!em)
        return;

    uint32 emote_id = em->textid;

    switch (emote_id)
    {
        case EMOTE_STATE_SLEEP:
        case EMOTE_STATE_SIT:
        case EMOTE_STATE_KNEEL:
        case EMOTE_ONESHOT_NONE:
            break;
        default:
        {
            // in feign death state allowed only text emotes.
            if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
                break;

            GetPlayer()->HandleEmoteCommand(emote_id);
            break;
        }
    }

    Unit* unit = GetPlayer()->GetMap()->GetUnit(guid);

    MaNGOS::EmoteChatBuilder emote_builder(*GetPlayer(), text_emote, emoteNum, unit);
    MaNGOS::LocalizedPacketDo<MaNGOS::EmoteChatBuilder > emote_do(emote_builder);
    MaNGOS::CameraDistWorker<MaNGOS::LocalizedPacketDo<MaNGOS::EmoteChatBuilder > > emote_worker(GetPlayer(), sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE), emote_do);
    Cell::VisitWorldObjects(GetPlayer(), emote_worker,  sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE));

    GetPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE, text_emote, 0, unit);

    // Send scripted event call
    if (unit && unit->GetTypeId() == TYPEID_UNIT && ((Creature*)unit)->AI())
        ((Creature*)unit)->AI()->ReceiveEmote(GetPlayer(), text_emote);
}

void WorldSession::HandleChatIgnoredOpcode(WorldPacket& recv_data)
{
    ObjectGuid iguid;
    uint8 unk;
    // DEBUG_LOG("WORLD: Received CMSG_CHAT_IGNORED");

    recv_data >> unk;                                       // probably related to spam reporting
    recv_data.ReadGuidMask<2, 5, 6, 4, 7, 0, 1, 3>(iguid);
    recv_data.ReadGuidBytes<0, 6, 5, 1, 4, 3, 7, 2>(iguid);

    Player* player = sObjectMgr.GetPlayer(iguid);
    if (!player || !player->GetSession())
        return;

    WorldPacket data;
    ChatHandler::FillMessageData(&data, this, CHAT_MSG_IGNORED, LANG_UNIVERSAL, NULL, GetPlayer()->GetObjectGuid(), GetPlayer()->GetName(), NULL);
    player->GetSession()->SendPacket(&data);
}

void WorldSession::SendPlayerNotFoundNotice(std::string name)
{
    WorldPacket data(SMSG_CHAT_PLAYER_NOT_FOUND, name.size() + 1);
    data << name;
    SendPacket(&data);
}

void WorldSession::SendPlayerAmbiguousNotice(std::string name)
{
    WorldPacket data(SMSG_CHAT_PLAYER_AMBIGUOUS, name.size() + 1);
    data << name;
    SendPacket(&data);
}

void WorldSession::SendWrongFactionNotice()
{
    WorldPacket data(SMSG_CHAT_WRONG_FACTION, 0);
    SendPacket(&data);
}

void WorldSession::SendChatRestrictedNotice(ChatRestrictionType restriction)
{
    WorldPacket data(SMSG_CHAT_RESTRICTED, 1);
    data << uint8(restriction);
    SendPacket(&data);
}
