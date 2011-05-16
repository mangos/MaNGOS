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

#include "CreatureAISelector.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "NullCreatureAI.h"
#include "Policies/SingletonImp.h"
#include "MovementGenerator.h"
#include "ScriptMgr.h"
#include "Pet.h"

INSTANTIATE_SINGLETON_1(CreatureAIRegistry);
INSTANTIATE_SINGLETON_1(MovementGeneratorRegistry);

namespace FactorySelector
{
    CreatureAI* selectAI(Creature *creature)
    {
        // Allow scripting AI for normal creatures and not controlled pets (guardians and mini-pets)
        if ((!creature->IsPet() || !((Pet*)creature)->isControlled()) && !creature->isCharmed())
            if (CreatureAI* scriptedAI = sScriptMgr.GetCreatureAI(creature))
                return scriptedAI;

        CreatureAIRegistry &ai_registry(CreatureAIRepository::Instance());

        const CreatureAICreator *ai_factory = NULL;

        std::string ainame=creature->GetAIName();

        // select by NPC flags _first_ - otherwise EventAI might be choosen for pets/totems
        // excplicit check for isControlled() and owner type to allow guardian, mini-pets and pets controlled by NPCs to be scripted by EventAI
        Unit *owner=NULL;
        if ((creature->IsPet() && ((Pet*)creature)->isControlled() &&
            ((owner=creature->GetOwner()) && owner->GetTypeId()==TYPEID_PLAYER)) || creature->isCharmed())
            ai_factory = ai_registry.GetRegistryItem("PetAI");
        else if (creature->IsTotem())
            ai_factory = ai_registry.GetRegistryItem("TotemAI");

        // select by script name
        if (!ai_factory && !ainame.empty())
            ai_factory = ai_registry.GetRegistryItem( ainame.c_str() );

        if (!ai_factory && creature->IsGuard())
            ai_factory = ai_registry.GetRegistryItem("GuardAI");

        // select by permit check
        if (!ai_factory)
        {
            int best_val = PERMIT_BASE_NO;
            typedef CreatureAIRegistry::RegistryMapType RMT;
            RMT const &l = ai_registry.GetRegisteredItems();
            for( RMT::const_iterator iter = l.begin(); iter != l.end(); ++iter)
            {
                const CreatureAICreator *factory = iter->second;
                const SelectableAI *p = dynamic_cast<const SelectableAI *>(factory);
                MANGOS_ASSERT( p != NULL );
                int val = p->Permit(creature);
                if( val > best_val )
                {
                    best_val = val;
                    ai_factory = p;
                }
            }
        }

        // select NullCreatureAI if not another cases
        ainame = (ai_factory == NULL) ? "NullCreatureAI" : ai_factory->key();

        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature %u used AI is %s.", creature->GetGUIDLow(), ainame.c_str() );
        return ( ai_factory == NULL ? new NullCreatureAI(creature) : ai_factory->Create(creature) );
    }

    MovementGenerator* selectMovementGenerator(Creature *creature)
    {
        MovementGeneratorRegistry &mv_registry(MovementGeneratorRepository::Instance());
        MANGOS_ASSERT( creature->GetCreatureInfo() != NULL );
        MovementGeneratorCreator const * mv_factory = mv_registry.GetRegistryItem(
            creature->GetOwnerGuid().IsPlayer() ? FOLLOW_MOTION_TYPE : creature->GetDefaultMovementType());

        /* if( mv_factory == NULL  )
        {
            int best_val = -1;
            std::vector<std::string> l;
            mv_registry.GetRegisteredItems(l);
            for( std::vector<std::string>::iterator iter = l.begin(); iter != l.end(); ++iter)
            {
            const MovementGeneratorCreator *factory = mv_registry.GetRegistryItem((*iter).c_str());
            const SelectableMovement *p = dynamic_cast<const SelectableMovement *>(factory);
            ASSERT( p != NULL );
            int val = p->Permit(creature);
            if( val > best_val )
            {
                best_val = val;
                mv_factory = p;
            }
            }
        }*/

        return ( mv_factory == NULL ? NULL : mv_factory->Create(creature) );

    }
}
