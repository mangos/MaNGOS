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

#ifndef MANGOS_INSTANCE_DATA_H
#define MANGOS_INSTANCE_DATA_H

#include "Common.h"

class Map;
class Unit;
class Player;
class GameObject;
class Creature;

class MANGOS_DLL_SPEC InstanceData
{
    public:

        explicit InstanceData(Map *map) : instance(map) {}
        virtual ~InstanceData() {}

        Map *instance;

        //On creation, NOT load.
        virtual void Initialize() {}

        //On load
        virtual void Load(const char* /*data*/) {}

        //When save is needed, this function generates the data
        virtual const char* Save() { return ""; }

        void SaveToDB();

        //Called every map update
        virtual void Update(uint32 /*diff*/) {}

        //Used by the map's CanEnter function.
        //This is to prevent players from entering during boss encounters.
        virtual bool IsEncounterInProgress() const { return false; };

        // used in InstanceBinding dialog
        virtual uint32 GetCompletedEncounters(bool /*type*/) { return 0; }

        //Called when a player successfully enters the instance (after really added to map)
        virtual void OnPlayerEnter(Player *) {}

        //Called when a player dies inside instance
        virtual void OnPlayerDeath(Player *) {}

        //Called when a player leaves the instance (before really removed from map (or possibly world))
        virtual void OnPlayerLeave(Player *) {}

        //Called when a gameobject is created
        virtual void OnObjectCreate(GameObject *) {}

        //called on creature creation
        virtual void OnCreatureCreate(Creature * /*creature*/) {}

        //called on creature enter combat
        virtual void OnCreatureEnterCombat(Creature * /*creature*/) {}

        //called on creature evade
        virtual void OnCreatureEvade(Creature * /*creature*/) {}

        //called on creature death
        virtual void OnCreatureDeath(Creature * /*creature*/) {}

        //All-purpose data storage 64 bit
        virtual uint64 GetData64(uint32 /*Data*/) { return 0; }
        virtual void SetData64(uint32 /*Data*/, uint64 /*Value*/) { }

        //All-purpose data storage 32 bit
        virtual uint32 GetData(uint32 /*Type*/) { return 0; }
        virtual void SetData(uint32 /*Type*/, uint32 /*Data*/) {}

        // Achievement criteria additional requirements check
        // NOTE: not use this if same can be checked existing requirement types from AchievementCriteriaRequirementType
        virtual bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* source, Unit const* target = NULL, uint32 miscvalue1 = 0);

        // Condition criteria additional requirements check
        // This is used for such things are heroic loot
        virtual bool CheckConditionCriteriaMeet(Player const* source, uint32 map_id, uint32 instance_condition_id);
};
#endif
