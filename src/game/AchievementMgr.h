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

#include "Player.h"

class AchievementMgr
{
    public:
        AchievementMgr(Player* pl);

        void LoadFromDB();
        void SaveToDB();
        void SendAchievementEarned(uint32 achievementId);
        void SendCriteriaUpdate(uint32 criteriaId, uint32 counter);

        Player* GetPlayer() { return m_player;}

    private:
        Player* m_player;
};


#endif
