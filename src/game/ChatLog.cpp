/*
 * Copyright (C) 2005,2006,2007 MaNGOS <http://www.mangosproject.org/>
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
#include "ChatLexicsCutter.h"
#include "ChatLog.h"
#include "Chat.h"
#include "Group.h"
#include "Guild.h"
#include "ObjectMgr.h"
#include "SpellAuras.h"
#include "Policies/SingletonImp.h"
#include "Config/Config.h"

INSTANTIATE_SINGLETON_1( ChatLog );

ChatLog::ChatLog()
{
    for (int i = 0; i <= CHATLOG_CHAT_TYPES_COUNT - 1; i++)
    {
        names[i] = "";
        files[i] = NULL;
    }

    Lexics = NULL;
    fn_innormative = "";
    f_innormative = NULL;

    Initialize();
}

ChatLog::~ChatLog()
{
    // close all files (avoiding double-close)
    CloseAllFiles();

    if (Lexics)
    {
        delete Lexics;
        Lexics = NULL;
    }
}

void ChatLog::Initialize()
{
    // determine, if the chat logs are enabled
    ChatLogEnable = sConfig.GetBoolDefault("ChatLogEnable", false);
    ChatLogDateSplit = sConfig.GetBoolDefault("ChatLogDateSplit", false);
    ChatLogUTFHeader = sConfig.GetBoolDefault("ChatLogUTFHeader", false);
    ChatLogIgnoreUnprintable = sConfig.GetBoolDefault("ChatLogIgnoreUnprintable", false);

    if (ChatLogEnable)
    {
        // read chat log file names
        names[CHAT_LOG_CHAT] = sConfig.GetStringDefault("ChatLogChatFile", "");
        names[CHAT_LOG_PARTY] = sConfig.GetStringDefault("ChatLogPartyFile", "");
        names[CHAT_LOG_GUILD] = sConfig.GetStringDefault("ChatLogGuildFile", "");
        names[CHAT_LOG_WHISPER] = sConfig.GetStringDefault("ChatLogWhisperFile", "");
        names[CHAT_LOG_CHANNEL] = sConfig.GetStringDefault("ChatLogChannelFile", "");
        names[CHAT_LOG_RAID] = sConfig.GetStringDefault("ChatLogRaidFile", "");
        names[CHAT_LOG_BATTLEGROUND] = sConfig.GetStringDefault("ChatLogBattleGroundFile", "");

        // read screen log flags
        screenflag[CHAT_LOG_CHAT] = sConfig.GetBoolDefault("ChatLogChatScreen", false);
        screenflag[CHAT_LOG_PARTY] = sConfig.GetBoolDefault("ChatLogPartyScreen", false);
        screenflag[CHAT_LOG_GUILD] = sConfig.GetBoolDefault("ChatLogGuildScreen", false);
        screenflag[CHAT_LOG_WHISPER] = sConfig.GetBoolDefault("ChatLogWhisperScreen", false);
        screenflag[CHAT_LOG_CHANNEL] = sConfig.GetBoolDefault("ChatLogChannelScreen", false);
        screenflag[CHAT_LOG_RAID] = sConfig.GetBoolDefault("ChatLogRaidScreen", false);
        screenflag[CHAT_LOG_BATTLEGROUND] = sConfig.GetBoolDefault("ChatLogBattleGroundScreen", false);
    }

    // lexics cutter
    LexicsCutterEnable = sConfig.GetBoolDefault("LexicsCutterEnable", false);

    if (LexicsCutterEnable)
    {
        // initialize lexics cutter parameters
        LexicsCutterInnormativeCut = sConfig.GetBoolDefault("LexicsCutterInnormativeCut", true);
        LexicsCutterNoActionOnGM = sConfig.GetBoolDefault("LexicsCutterNoActionOnGM", true);
        LexicsCutterScreenLog = sConfig.GetBoolDefault("LexicsCutterScreenLog", false);
        LexicsCutterCutReplacement = sConfig.GetStringDefault("LexicsCutterCutReplacement", "&!@^%!^&*!!! [gibberish]");
        LexicsCutterAction = sConfig.GetIntDefault("LexicsCutterAction", 0);
        LexicsCutterActionDuration = sConfig.GetIntDefault("LexicsCutterActionDuration", 60000);
        std::string fn_analogsfile = sConfig.GetStringDefault("LexicsCutterAnalogsFile", "");
        std::string fn_wordsfile = sConfig.GetStringDefault("LexicsCutterWordsFile", "");

        // read lexics cutter flags
        cutflag[CHAT_LOG_CHAT] = sConfig.GetBoolDefault("LexicsCutInChat", true);
        cutflag[CHAT_LOG_PARTY] = sConfig.GetBoolDefault("LexicsCutInParty", true);
        cutflag[CHAT_LOG_GUILD] = sConfig.GetBoolDefault("LexicsCutInGuild", true);
        cutflag[CHAT_LOG_WHISPER] = sConfig.GetBoolDefault("LexicsCutInWhisper", true);
        cutflag[CHAT_LOG_CHANNEL] = sConfig.GetBoolDefault("LexicsCutInChannel", true);
        cutflag[CHAT_LOG_RAID] = sConfig.GetBoolDefault("LexicsCutInRaid", true);
        cutflag[CHAT_LOG_BATTLEGROUND] = sConfig.GetBoolDefault("LexicsCutInBattleGround", true);

        if (fn_analogsfile == "" || fn_wordsfile == "")
        {
            LexicsCutterEnable = false;
        }
        else
        {
            // initialize lexics cutter
            Lexics = new LexicsCutter;
            if (Lexics) Lexics->Read_Letter_Analogs(fn_analogsfile);
            if (Lexics) Lexics->Read_Innormative_Words(fn_wordsfile);
            if (Lexics) Lexics->Map_Innormative_Words();

            // read additional parameters
            Lexics->IgnoreLetterRepeat = sConfig.GetBoolDefault("LexicsCutterIgnoreRepeats", true);
            Lexics->IgnoreMiddleSpaces = sConfig.GetBoolDefault("LexicsCutterIgnoreSpaces", true);
            fn_innormative = sConfig.GetStringDefault("LexicsCutterLogFile", "");
        }
    }

    // open all files (with aliasing)
    OpenAllFiles();

    // write timestamps (init)
    WriteInitStamps();
}

bool ChatLog::_ChatCommon(int ChatType, Player *player, std::string &msg)
{
    if (LexicsCutterEnable && Lexics && cutflag[ChatType] && Lexics->Check_Lexics(msg)) ChatBadLexicsAction(player, msg);

    if (!ChatLogEnable) return(false);

    if (ChatLogIgnoreUnprintable)
    {
        // have to ignore unprintables, verify string by UTF8 here
        unsigned int pos = 0;
        std::string lchar;
        while (LexicsCutter::ReadUTF8(msg, lchar, pos))
        {
            if (lchar.size() == 1)
            {
                if (lchar[0] < ' ') return(false); // unprintable detected
            }
        }
    }

    return(true);
}

void ChatLog::ChatMsg(Player *player, std::string &msg, uint32 type)
{
    if (!_ChatCommon(CHAT_LOG_CHAT, player, msg)) return;

    CheckDateSwitch();

    std::string log_str = "";

    switch (type)
    {
        case CHAT_MSG_EMOTE:
        log_str.append("{EMOTE} ");
        break;

        case CHAT_MSG_YELL:
        log_str.append("{YELL} ");
        break;
    }

    log_str.append("[");
    log_str.append(player->GetName());
    log_str.append("] ");

    log_str.append(msg);

    log_str.append("\n");

    if (screenflag[CHAT_LOG_CHAT]) printf("%s", log_str.c_str());
    if (files[CHAT_LOG_CHAT])
    {
        OutTimestamp(files[CHAT_LOG_CHAT]);
        fprintf(files[CHAT_LOG_CHAT], "%s", log_str.c_str());
        fflush(files[CHAT_LOG_CHAT]);
    }
}

void ChatLog::PartyMsg(Player *player, std::string &msg)
{
    if (!_ChatCommon(CHAT_LOG_PARTY, player, msg)) return;

    CheckDateSwitch();

    std::string log_str = "";

    log_str.append("[");
    log_str.append(player->GetName());
    log_str.append("]->GROUP:");

    Group *group = player->GetGroup();
    if (!group)
    {
        log_str.append("[unknown group] ");
    }
    else
    {
        // obtain group information
        log_str.append("[");

        uint8 gm_count = group->GetMembersCount();
        uint8 gm_count_m1 = gm_count - 1;
        uint64 gm_leader_GUID = group->GetLeaderGUID();
        Player *gm_member;

        gm_member = sObjectMgr.GetPlayer(gm_leader_GUID);
        if (gm_member)
        {
            log_str.append(gm_member->GetName());
            log_str.append(",");
        }

        Group::MemberSlotList g_members = group->GetMemberSlots();

        for (Group::member_citerator itr = g_members.begin(); itr != g_members.end(); itr++)
        {
            if (itr->guid == gm_leader_GUID) continue;

            gm_member = sObjectMgr.GetPlayer(itr->guid);
            if (gm_member)
            {
                log_str.append(itr->name);
                log_str.append(",");
            }
        }

        log_str.erase(log_str.length() - 1);
        log_str.append("] ");
    }

    log_str.append(msg);

    log_str.append("\n");

    if (screenflag[CHAT_LOG_PARTY]) printf("%s", log_str.c_str());
    if (files[CHAT_LOG_PARTY])
    {
        OutTimestamp(files[CHAT_LOG_PARTY]);
        fprintf(files[CHAT_LOG_PARTY], "%s", log_str.c_str());
        fflush(files[CHAT_LOG_PARTY]);
    }
}

void ChatLog::GuildMsg(Player *player, std::string &msg, bool officer)
{
    if (!_ChatCommon(CHAT_LOG_GUILD, player, msg)) return;

    CheckDateSwitch();

    std::string log_str = "";

    log_str.append("[");
    log_str.append(player->GetName());
    log_str.append((officer ? "]->GUILD_OFF:" : "]->GUILD:"));

    if (!player->GetGuildId())
    {
        log_str.append("[unknown guild] ");
    }
    else
    {
        Guild *guild = sObjectMgr.GetGuildById(player->GetGuildId());
        if (!guild)
        {
            log_str.append("[unknown guild] ");
        }
        else
        {
            // obtain guild information
            log_str.append("(");
            log_str.append(guild->GetName());
            log_str.append(") ");
        }
    }

    log_str.append(msg);

    log_str.append("\n");

    if (screenflag[CHAT_LOG_GUILD]) printf("%s", log_str.c_str());
    if (files[CHAT_LOG_GUILD])
    {
        OutTimestamp(files[CHAT_LOG_GUILD]);
        fprintf(files[CHAT_LOG_GUILD], "%s", log_str.c_str());
        fflush(files[CHAT_LOG_GUILD]);
    }
}

void ChatLog::WhisperMsg(Player *player, std::string &to, std::string &msg)
{
    if (!_ChatCommon(CHAT_LOG_WHISPER, player, msg)) return;

    CheckDateSwitch();

    std::string log_str = "";

    log_str.append("[");
    log_str.append(player->GetName());
    log_str.append("]->");

    if (to.size() == 0)
    {
        log_str.append("[???] ");
    }
    else
    {
        normalizePlayerName(to);
        log_str.append("[");
        log_str.append(to);
        log_str.append("] ");
    }

    log_str.append(msg);

    log_str.append("\n");

    if (screenflag[CHAT_LOG_WHISPER]) printf("%s", log_str.c_str());
    if (files[CHAT_LOG_WHISPER])
    {
        OutTimestamp(files[CHAT_LOG_WHISPER]);
        fprintf(files[CHAT_LOG_WHISPER], "%s", log_str.c_str());
        fflush(files[CHAT_LOG_WHISPER]);
    }
}

void ChatLog::ChannelMsg(Player *player, std::string &channel, std::string &msg)
{
    if (!_ChatCommon(CHAT_LOG_CHANNEL, player, msg)) return;

    CheckDateSwitch();

    std::string log_str = "";

    log_str.append("[");
    log_str.append(player->GetName());
    log_str.append("]->CHANNEL:");

    if (channel.size() == 0)
    {
        log_str.append("[unknown channel] ");
    }
    else
    {
        log_str.append("[");
        log_str.append(channel);
        log_str.append("] ");
    }

    log_str.append(msg);

    log_str.append("\n");

    if (screenflag[CHAT_LOG_CHANNEL]) printf("%s", log_str.c_str());
    if (files[CHAT_LOG_CHANNEL])
    {
        OutTimestamp(files[CHAT_LOG_CHANNEL]);
        fprintf(files[CHAT_LOG_CHANNEL], "%s", log_str.c_str());
        fflush(files[CHAT_LOG_CHANNEL]);
    }
}

void ChatLog::RaidMsg(Player *player, std::string &msg, uint32 type)
{
    if (!_ChatCommon(CHAT_LOG_RAID, player, msg)) return;

    CheckDateSwitch();

    std::string log_str = "";

    log_str.append("[");
    log_str.append(player->GetName());

    switch (type)
    {
        case CHAT_MSG_RAID:
        log_str.append("]->RAID:");
        break;

        case CHAT_MSG_RAID_LEADER:
        log_str.append("]->RAID_LEADER:");
        break;

        case CHAT_MSG_RAID_WARNING:
        log_str.append("]->RAID_WARN:");
        break;

        default:
        log_str.append("]->RAID_UNKNOWN:");
    }

    Group *group = player->GetGroup();
    if (!group)
    {
        log_str.append("[unknown raid] ");
    }
    else
    {
        // obtain group information
        log_str.append("[");

        uint8 gm_count = group->GetMembersCount();
        uint8 gm_count_m1 = gm_count - 1;
        uint64 gm_leader_GUID = group->GetLeaderGUID();
        Player *gm_member;

        gm_member = sObjectMgr.GetPlayer(gm_leader_GUID);
        if (gm_member)
        {
            log_str.append(gm_member->GetName());
            log_str.append(",");
        }

        Group::MemberSlotList g_members = group->GetMemberSlots();

        for (Group::member_citerator itr = g_members.begin(); itr != g_members.end(); itr++)
        {
            if (itr->guid == gm_leader_GUID) continue;

            gm_member = sObjectMgr.GetPlayer(itr->guid);
            if (gm_member)
            {
                log_str.append(itr->name);
                log_str.append(",");
            }
        }

        log_str.erase(log_str.length() - 1);
        log_str.append("] ");
    }

    log_str.append(msg);

    log_str.append("\n");

    if (screenflag[CHAT_LOG_RAID]) printf("%s", log_str.c_str());
    if (files[CHAT_LOG_RAID])
    {
        OutTimestamp(files[CHAT_LOG_RAID]);
        fprintf(files[CHAT_LOG_RAID], "%s", log_str.c_str());
        fflush(files[CHAT_LOG_RAID]);
    }
}

void ChatLog::BattleGroundMsg(Player *player, std::string &msg, uint32 type)
{
    if (!_ChatCommon(CHAT_LOG_BATTLEGROUND, player, msg)) return;

    CheckDateSwitch();

    std::string log_str = "";

    log_str.append("[");
    log_str.append(player->GetName());

    switch (type)
    {
        case CHAT_MSG_BATTLEGROUND:
        log_str.append("]->BG:");
        break;

        case CHAT_MSG_BATTLEGROUND_LEADER:
        log_str.append("]->BG_LEADER:");
        break;

        default:
        log_str.append("]->BG_UNKNOWN:");
    }

    Group *group = player->GetGroup();
    if (!group)
    {
        log_str.append("[unknown group] ");
    }
    else
    {
        // obtain group information
        log_str.append("[");

        uint8 gm_count = group->GetMembersCount();
        uint8 gm_count_m1 = gm_count - 1;
        uint64 gm_leader_GUID = group->GetLeaderGUID();
        Player *gm_member;

        gm_member = sObjectMgr.GetPlayer(gm_leader_GUID);
        if (gm_member)
        {
            log_str.append(gm_member->GetName());
            log_str.append(",");
        }

        Group::MemberSlotList g_members = group->GetMemberSlots();

        for (Group::member_citerator itr = g_members.begin(); itr != g_members.end(); itr++)
        {
            if (itr->guid == gm_leader_GUID) continue;

            gm_member = sObjectMgr.GetPlayer(itr->guid);
            if (gm_member)
            {
                log_str.append(itr->name);
                log_str.append(",");
            }
        }

        log_str.erase(log_str.length() - 1);
        log_str.append("] ");
    }

    log_str.append(msg);

    log_str.append("\n");

    if (screenflag[CHAT_LOG_BATTLEGROUND]) printf("%s", log_str.c_str());
    if (files[CHAT_LOG_BATTLEGROUND])
    {
        OutTimestamp(files[CHAT_LOG_BATTLEGROUND]);
        fprintf(files[CHAT_LOG_BATTLEGROUND], "%s", log_str.c_str());
        fflush(files[CHAT_LOG_BATTLEGROUND]);
    }
}

void ChatLog::OpenAllFiles()
{
    std::string tempname;
    char dstr[12];

    if (ChatLogDateSplit)
    {
        time_t t = time(NULL);
        tm* aTm = localtime(&t);
        sprintf(dstr, "%-4d-%02d-%02d", aTm->tm_year + 1900, aTm->tm_mon + 1, aTm->tm_mday);
    }

    if (ChatLogEnable)
    {
        for (int i = 0; i <= CHATLOG_CHAT_TYPES_COUNT - 1; i++)
        {
            if (names[i] != "")
            {
                for (int j = i - 1; j >= 0; j--)
                {
                    if (names[i] == names[j])
                    {
                        files[i] = files[j];
                        break;
                    }
                }
                if (!files[i])
                {
                    tempname = names[i];
                    if (ChatLogDateSplit)
                    {
                        // append date instead of $d if applicable
                        int dpos = tempname.find("$d");
                        if (dpos != tempname.npos)
                        {
                            tempname.replace(dpos, 2, &dstr[0], 10);
                        }
                    }
                    files[i] = fopen(tempname.c_str(), "a+b");
                    if (ChatLogUTFHeader && (ftell(files[i]) == 0)) fputs("\xEF\xBB\xBF", files[i]);
                }
            }
        }
    }

    // initialize innormative log
    if (LexicsCutterEnable)
    {
        if (fn_innormative != "")
        {
            tempname = fn_innormative;
            if (ChatLogDateSplit)
            {
                // append date instead of $d if applicable
                int dpos = tempname.find("$d");
                if (dpos != tempname.npos)
                {
                    tempname.replace(dpos, 2, &dstr[0], 10);
                }
            }
            f_innormative = fopen(tempname.c_str(), "a+b");
            if (ChatLogUTFHeader && (ftell(f_innormative) == 0)) fputs("\xEF\xBB\xBF", f_innormative);
        }
    }
}

void ChatLog::CloseAllFiles()
{
    for (int i = 0; i <= CHATLOG_CHAT_TYPES_COUNT - 1; i++)
    {
        if (files[i])
        {
            for (int j = i + 1; j <= CHATLOG_CHAT_TYPES_COUNT - 1; j++)
            {
                if (files[j] == files[i]) files[j] = NULL;
            }

            fclose(files[i]);
            files[i] = NULL;
        }
    }

    if (f_innormative)
    {
        fclose(f_innormative);
        f_innormative = NULL;
    }
}

void ChatLog::CheckDateSwitch()
{
    if (ChatLogDateSplit)
    {
        time_t t = time(NULL);
        tm* aTm = localtime(&t);
        if (lastday != aTm->tm_mday)
        {
            // date switched
            CloseAllFiles();
            OpenAllFiles();
            WriteInitStamps();
        }
    }
}

void ChatLog::WriteInitStamps()
{
    // remember date
    time_t t = time(NULL);
    tm* aTm = localtime(&t);
    lastday = aTm->tm_mday;

    if (files[CHAT_LOG_CHAT])
    {
        OutTimestamp(files[CHAT_LOG_CHAT]);
        fprintf(files[CHAT_LOG_CHAT], "%s", "[SYSTEM] Chat Log Initialized\n");
    }
    if (files[CHAT_LOG_PARTY])
    {
        OutTimestamp(files[CHAT_LOG_PARTY]);
        fprintf(files[CHAT_LOG_PARTY], "%s", "[SYSTEM] Party Chat Log Initialized\n");
    }
    if (files[CHAT_LOG_GUILD])
    {
        OutTimestamp(files[CHAT_LOG_GUILD]);
        fprintf(files[CHAT_LOG_GUILD], "%s", "[SYSTEM] Guild Chat Log Initialized\n");
    }
    if (files[CHAT_LOG_WHISPER])
    {
        OutTimestamp(files[CHAT_LOG_WHISPER]);
        fprintf(files[CHAT_LOG_WHISPER], "%s", "[SYSTEM] Whisper Log Initialized\n");
    }
    if (files[CHAT_LOG_CHANNEL])
    {
        OutTimestamp(files[CHAT_LOG_CHANNEL]);
        fprintf(files[CHAT_LOG_CHANNEL], "%s", "[SYSTEM] Chat Channels Log Initialized\n");
    }
    if (files[CHAT_LOG_RAID])
    {
        OutTimestamp(files[CHAT_LOG_RAID]);
        fprintf(files[CHAT_LOG_RAID], "%s", "[SYSTEM] Raid Party Chat Log Initialized\n");
    }

    if (f_innormative)
    {
        OutTimestamp(f_innormative);
        fprintf(f_innormative, "%s", "[SYSTEM] Innormative Lexics Log Initialized\n");
    }
}

void ChatLog::OutTimestamp(FILE* file)
{
    time_t t = time(NULL);
    tm* aTm = localtime(&t);
    fprintf(file, "%-4d-%02d-%02d %02d:%02d:%02d ", aTm->tm_year + 1900, aTm->tm_mon + 1, aTm->tm_mday, aTm->tm_hour, aTm->tm_min, aTm->tm_sec);
}

void ChatLog::ChatBadLexicsAction(Player* player, std::string& msg)
{
    // logging
    std::string log_str = "";

    log_str.append("[");
    log_str.append(player->GetName());
    log_str.append("] ");

    log_str.append(msg);

    log_str.append("\n");

    if (LexicsCutterScreenLog) printf("<INNORMATIVE!> %s", log_str.c_str());
    if (f_innormative)
    {
        OutTimestamp(f_innormative);
        fprintf(f_innormative, "%s", log_str.c_str());
        fflush(f_innormative);
    }

    // cutting innormative lexics
    if (LexicsCutterInnormativeCut)
    {
        msg = LexicsCutterCutReplacement;
    }

    if (!player || !player->GetSession()) return;

    if (LexicsCutterNoActionOnGM && player->GetSession()->GetSecurity()) return;

    // special action
    const SpellEntry* sl;

    switch (LexicsCutterAction)
    {
        case LEXICS_ACTION_SHEEP:
        {
            // sheep me, yeah, yeah, sheep me
            sl = sSpellStore.LookupEntry(118);
            if (sl)
            {
                for (int i = 0; i < MAX_EFFECT_INDEX; i++)
                {
                    Aura* Aur = CreateAura(sl, SpellEffectIndex(i), NULL, player);
                    if (Aur)
                    {
                        Aur->SetAuraDuration(LexicsCutterActionDuration);
                        player->AddAura(Aur);
                    }
                }
            }
        }
        break;

        case LEXICS_ACTION_STUN:
        {
            // stunned surprised
            sl = sSpellStore.LookupEntry(13005);
            if (sl)
            {
                for (int i = 0; i < MAX_EFFECT_INDEX; i++)
                {
                    Aura* Aur = CreateAura(sl, SpellEffectIndex(i), NULL, player);
                    if (Aur)
                    {
                        Aur->SetAuraDuration(LexicsCutterActionDuration);
                        player->AddAura(Aur);
                    }
                }
            }
        }
        break;

        case LEXICS_ACTION_DIE:
        {
            // oops, kicked the bucket
            player->DealDamage(player, player->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        }
        break;

        case LEXICS_ACTION_DRAIN:
        {
            // living corpse :)
            player->DealDamage(player, player->GetHealth() - 5, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        }
        break;

        case LEXICS_ACTION_SILENCE:
        {
            // glue the mouth
            time_t mutetime = time(NULL) + (int) (LexicsCutterActionDuration / 1000);
            player->GetSession()->m_muteTime = mutetime;
        }
        break;

        case LEXICS_ACTION_STUCK:
        {
            // yo, the Matrix has had you :) [by KAPATEJIb]
            sl = sSpellStore.LookupEntry(23312);
            if (sl)
            {
                for (int i = 0; i < MAX_EFFECT_INDEX; i++)
                {
                    Aura* Aur = CreateAura(sl, SpellEffectIndex(i), NULL, player);
                    if (Aur)
                    {
                        Aur->SetAuraDuration(LexicsCutterActionDuration);
                        player->AddAura(Aur);
                    }
                }
            }
        }
        break;

        case LEXICS_ACTION_SICKNESS:
        {
            // for absence of censorship, there is punishment [by Koshei]
            sl = sSpellStore.LookupEntry(15007);
            if (sl)
            {
                for (int i = 0; i < MAX_EFFECT_INDEX; i++)
                {
                    Aura* Aur = CreateAura(sl, SpellEffectIndex(i), NULL, player);
                    if (Aur)
                    {
                        Aur->SetAuraDuration(LexicsCutterActionDuration);
                        player->AddAura(Aur);
                    }
                }
            }
        }
        break;

        case LEXICS_ACTION_SHEAR:
        {
            // Lord Illidan to watch you [by Koshei]
            sl = sSpellStore.LookupEntry(41032);
            if (sl)
            {
                for (int i = 0; i < MAX_EFFECT_INDEX; i++)
                {
                    Aura* Aur = CreateAura(sl, SpellEffectIndex(i), NULL, player);
                    if (Aur)
                    {
                        Aur->SetAuraDuration(LexicsCutterActionDuration);
                        player->AddAura(Aur);
                    }
                }
            }
        }
        break;

        default:
        // no action except logging
        break;
    }
}
