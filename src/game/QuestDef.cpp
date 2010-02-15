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

#include "QuestDef.h"
#include "Player.h"
#include "World.h"

Quest::Quest(Field * questRecord)
{
    QuestId = questRecord[0].GetUInt32();
    QuestMethod = questRecord[1].GetUInt32();
    ZoneOrSort = questRecord[2].GetInt32();
    SkillOrClass = questRecord[3].GetInt32();
    MinLevel = questRecord[4].GetUInt32();
    QuestLevel = questRecord[5].GetInt32();
    Type = questRecord[6].GetUInt32();
    RequiredRaces = questRecord[7].GetUInt32();
    RequiredSkillValue = questRecord[8].GetUInt32();
    RepObjectiveFaction = questRecord[9].GetUInt32();
    RepObjectiveValue = questRecord[10].GetInt32();
    RequiredMinRepFaction = questRecord[11].GetUInt32();
    RequiredMinRepValue = questRecord[12].GetInt32();
    RequiredMaxRepFaction = questRecord[13].GetUInt32();
    RequiredMaxRepValue = questRecord[14].GetInt32();
    SuggestedPlayers = questRecord[15].GetUInt32();
    LimitTime = questRecord[16].GetUInt32();
    QuestFlags = questRecord[17].GetUInt16();
    uint32 SpecialFlags = questRecord[18].GetUInt16();
    CharTitleId = questRecord[19].GetUInt32();
    PlayersSlain = questRecord[20].GetUInt32();
    BonusTalents = questRecord[21].GetUInt32();
    PrevQuestId = questRecord[22].GetInt32();
    NextQuestId = questRecord[23].GetInt32();
    ExclusiveGroup = questRecord[24].GetInt32();
    NextQuestInChain = questRecord[25].GetUInt32();
    RewXPId = questRecord[26].GetUInt32();
    SrcItemId = questRecord[27].GetUInt32();
    SrcItemCount = questRecord[28].GetUInt32();
    SrcSpell = questRecord[29].GetUInt32();
    Title = questRecord[30].GetCppString();
    Details = questRecord[31].GetCppString();
    Objectives = questRecord[32].GetCppString();
    OfferRewardText = questRecord[33].GetCppString();
    RequestItemsText = questRecord[34].GetCppString();
    EndText = questRecord[35].GetCppString();
    CompletedText = questRecord[36].GetCppString();

    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        ObjectiveText[i] = questRecord[37+i].GetCppString();

    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        ReqItemId[i] = questRecord[41+i].GetUInt32();

    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        ReqItemCount[i] = questRecord[47+i].GetUInt32();

    for (int i = 0; i < QUEST_SOURCE_ITEM_IDS_COUNT; ++i)
        ReqSourceId[i] = questRecord[53+i].GetUInt32();

    for (int i = 0; i < QUEST_SOURCE_ITEM_IDS_COUNT; ++i)
        ReqSourceCount[i] = questRecord[57+i].GetUInt32();

    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        ReqCreatureOrGOId[i] = questRecord[61+i].GetInt32();

    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        ReqCreatureOrGOCount[i] = questRecord[65+i].GetUInt32();

    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        ReqSpell[i] = questRecord[69+i].GetUInt32();

    for (int i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        RewChoiceItemId[i] = questRecord[73+i].GetUInt32();

    for (int i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        RewChoiceItemCount[i] = questRecord[79+i].GetUInt32();

    for (int i = 0; i < QUEST_REWARDS_COUNT; ++i)
        RewItemId[i] = questRecord[85+i].GetUInt32();

    for (int i = 0; i < QUEST_REWARDS_COUNT; ++i)
        RewItemCount[i] = questRecord[89+i].GetUInt32();

    for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
        RewRepFaction[i] = questRecord[93+i].GetUInt32();

    for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
        RewRepValueId[i] = questRecord[98+i].GetInt32();

    for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
        RewRepValue[i] = questRecord[103+i].GetInt32();

    RewHonorAddition = questRecord[108].GetUInt32();
    RewHonorMultiplier = questRecord[109].GetFloat();
    RewOrReqMoney = questRecord[110].GetInt32();
    RewMoneyMaxLevel = questRecord[111].GetUInt32();
    RewSpell = questRecord[112].GetUInt32();
    RewSpellCast = questRecord[113].GetUInt32();
    RewMailTemplateId = questRecord[114].GetUInt32();
    RewMailDelaySecs = questRecord[115].GetUInt32();
    PointMapId = questRecord[116].GetUInt32();
    PointX = questRecord[117].GetFloat();
    PointY = questRecord[118].GetFloat();
    PointOpt = questRecord[119].GetUInt32();

    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        DetailsEmote[i] = questRecord[120+i].GetUInt32();

    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        DetailsEmoteDelay[i] = questRecord[124+i].GetUInt32();

    IncompleteEmote = questRecord[128].GetUInt32();
    CompleteEmote = questRecord[129].GetUInt32();

    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        OfferRewardEmote[i] = questRecord[130+i].GetInt32();

    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        OfferRewardEmoteDelay[i] = questRecord[134+i].GetInt32();

    QuestStartScript = questRecord[138].GetUInt32();
    QuestCompleteScript = questRecord[139].GetUInt32();

    QuestFlags |= SpecialFlags << 24;

    m_reqitemscount = 0;
    m_reqCreatureOrGOcount = 0;
    m_rewitemscount = 0;
    m_rewchoiceitemscount = 0;

    for (int i=0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
    {
        if ( ReqItemId[i] )
            ++m_reqitemscount;
    }

    for (int i=0; i < QUEST_OBJECTIVES_COUNT; ++i)
    {
        if ( ReqCreatureOrGOId[i] )
            ++m_reqCreatureOrGOcount;
    }

    for (int i=0; i < QUEST_REWARDS_COUNT; ++i)
    {
        if ( RewItemId[i] )
            ++m_rewitemscount;
    }

    for (int i=0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
    {
        if (RewChoiceItemId[i])
            ++m_rewchoiceitemscount;
    }
}

uint32 Quest::XPValue(Player *pPlayer) const
{
    if (pPlayer)
    {
        uint32 realXP = 0;
        uint32 xpMultiplier = 0;
        int32 baseLevel = 0;
        int32 playerLevel = pPlayer->getLevel();

        // formula can possibly be organized better, using less if's and simplify some.

        if (QuestLevel != -1)
            baseLevel = QuestLevel;

        if (((baseLevel - playerLevel) + 10)*2 > 10)
        {
            baseLevel = playerLevel;

            if (QuestLevel != -1)
                baseLevel = QuestLevel;

            if (((baseLevel - playerLevel) + 10)*2 <= 10)
            {
                if (QuestLevel == -1)
                    baseLevel = playerLevel;

                xpMultiplier = 2 * (baseLevel - playerLevel) + 20;
            }
            else
            {
                xpMultiplier = 10;
            }
        }
        else
        {
            baseLevel = playerLevel;

            if (QuestLevel != -1)
                baseLevel = QuestLevel;

            if (((baseLevel - playerLevel) + 10)*2 >= 1)
            {
                baseLevel = playerLevel;

                if (QuestLevel != -1)
                    baseLevel = QuestLevel;

                if (((baseLevel - playerLevel) + 10)*2 <= 10)
                {
                    if (QuestLevel == -1)
                        baseLevel = playerLevel;

                    xpMultiplier = 2 * (baseLevel - playerLevel) + 20;
                }
                else
                {
                    xpMultiplier = 10;
                }
            }
            else
            {
                xpMultiplier = 1;
            }
        }

        // not possible to reward XP when baseLevel does not exist in dbc
        if (const QuestXPLevel* pXPData = sQuestXPLevelStore.LookupEntry(baseLevel))
        {
            uint32 rawXP = xpMultiplier * pXPData->xpIndex[RewXPId] / 10;

            // round values
            if (rawXP > 1000)
                realXP = ((rawXP + 25) / 50 * 50);
            else if (rawXP > 500)
                realXP = ((rawXP + 12) / 25 * 25);
            else if (rawXP > 100)
                realXP = ((rawXP + 5) / 10 * 10);
            else
                realXP = ((rawXP + 2) / 5 * 5);
        }

        return realXP;
    }

    return 0;
}

int32  Quest::GetRewOrReqMoney() const
{
    if(RewOrReqMoney <=0)
        return RewOrReqMoney;

    return int32(RewOrReqMoney * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
}
