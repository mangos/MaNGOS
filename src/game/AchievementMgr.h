/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
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
#ifndef __MANGOS_ACHIEVEMENTMGR_H
#define __MANGOS_ACHIEVEMENTMGR_H

#include "Common.h"
#include "Database/DBCEnums.h"
#include "Database/DBCStores.h"

typedef HM_NAMESPACE::hash_map<uint32, uint32> CriteriaProgressMap;
typedef HM_NAMESPACE::hash_map<uint32, time_t> CompletedAchievementMap;

class Player;
class WorldPacket;

class AchievementMgr
{
    public:
        AchievementMgr(Player* pl);

        void LoadFromDB();
        void SaveToDB();
        void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1=0, uint32 miscvalue2=0, uint32 time=0);
        void CheckAllAchievementCriteria();
        void SendAllAchievementData();
        void SendRespondInspectAchievements(Player* player);

        Player* GetPlayer() { return m_player;}

    private:
        void SendAchievementEarned(uint32 achievementId);
        void SendCriteriaUpdate(uint32 criteriaId, uint32 counter);
        void SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 newValue);
        void CompletedCriteria(AchievementCriteriaEntry const* entry);
        void CompletedAchievement(AchievementEntry const* entry);
        bool IsCompletedCriteria(AchievementCriteriaEntry const* entry);
        bool IsCompletedAchievement(AchievementEntry const* entry);
        void BuildAllDataPacket(WorldPacket *data);

        Player* m_player;
        CriteriaProgressMap m_criteriaProgress;
        CompletedAchievementMap m_completedAchievements;
};


#endif
