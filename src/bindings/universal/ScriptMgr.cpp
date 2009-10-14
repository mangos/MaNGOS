/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "config.h"
#include "ScriptMgr.h"
#include "../../game/GossipDef.h"
#include "../../game/GameObject.h"
#include "../../game/Player.h"
#include "../../game/Map.h"
#include "../../game/ObjectMgr.h"

//uint8 loglevel = 0;
int nrscripts;
Script *m_scripts[MAX_SCRIPTS];

// -- Scripts to be added --
extern void AddSC_default();
// -------------------

MANGOS_DLL_EXPORT
void ScriptsFree()
{                                                           // Free resources before library unload
    for(int i=0;i<nrscripts;i++)
        delete m_scripts[i];

    nrscripts = 0;
}

MANGOS_DLL_EXPORT
void ScriptsInit()
{
    nrscripts = GetScriptNames().size();
    for(int i=0;i<MAX_SCRIPTS;i++)
    {
         m_scripts[i]=NULL;
    }

    // -- Inicialize the Scripts to be Added --
    AddSC_default();
    // ----------------------------------------

}

MANGOS_DLL_EXPORT
char const* ScriptsVersion()
{
    return "Default MaNGOS scripting library";
}

void Script::registerSelf()
{
    int id = GetScriptId(Name.c_str());
    if(id != 0) m_scripts[id] = this;
}

MANGOS_DLL_EXPORT
bool GossipHello ( Player * player, Creature *_Creature )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pGossipHello) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGossipHello(player,_Creature);
}

MANGOS_DLL_EXPORT
bool GossipSelect( Player *player, Creature *_Creature,uint32 sender, uint32 action )
{
    debug_log("DEBUG: Gossip selection, sender: %d, action: %d",sender, action);

    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pGossipSelect) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGossipSelect(player,_Creature,sender,action);
}

MANGOS_DLL_EXPORT
bool GossipSelectWithCode( Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* sCode )
{
    debug_log("DEBUG: Gossip selection, sender: %d, action: %d",sender, action);

    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pGossipSelectWithCode) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGossipSelectWithCode(player,_Creature,sender,action,sCode);
}

MANGOS_DLL_EXPORT
bool QuestAccept( Player *player, Creature *_Creature, Quest *_Quest )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pQuestAccept) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pQuestAccept(player,_Creature,_Quest);
}

MANGOS_DLL_EXPORT
bool QuestSelect( Player *player, Creature *_Creature, Quest *_Quest )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pQuestSelect) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pQuestSelect(player,_Creature,_Quest);
}

MANGOS_DLL_EXPORT
bool QuestComplete( Player *player, Creature *_Creature, Quest *_Quest )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pQuestComplete) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pQuestComplete(player,_Creature,_Quest);
}

MANGOS_DLL_EXPORT
bool ChooseReward( Player *player, Creature *_Creature, Quest *_Quest, uint32 opt )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pChooseReward) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pChooseReward(player,_Creature,_Quest,opt);
}

MANGOS_DLL_EXPORT
uint32 NPCDialogStatus( Player *player, Creature *_Creature )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->pNPCDialogStatus) return 100;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pNPCDialogStatus(player,_Creature);
}

MANGOS_DLL_EXPORT
uint32 GODialogStatus( Player *player, GameObject *_GO )
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if(!tmpscript || !tmpscript->pGODialogStatus) return 100;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGODialogStatus(player,_GO);
}

MANGOS_DLL_EXPORT
bool ItemHello( Player *player, Item *_Item, Quest *_Quest )
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[_Item->GetProto()->ScriptId];
    if(!tmpscript || !tmpscript->pItemHello) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pItemHello(player,_Item,_Quest);
}

MANGOS_DLL_EXPORT
bool ItemQuestAccept( Player *player, Item *_Item, Quest *_Quest )
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[_Item->GetProto()->ScriptId];
    if(!tmpscript || !tmpscript->pItemQuestAccept) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pItemQuestAccept(player,_Item,_Quest);
}

MANGOS_DLL_EXPORT
bool GOHello( Player *player, GameObject *_GO )
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if(!tmpscript || !tmpscript->pGOHello) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOHello(player,_GO);
}

MANGOS_DLL_EXPORT
bool GOQuestAccept( Player *player, GameObject *_GO, Quest *_Quest )
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if(!tmpscript || !tmpscript->pGOQuestAccept) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOQuestAccept(player,_GO,_Quest);
}

MANGOS_DLL_EXPORT
bool GOChooseReward( Player *player, GameObject *_GO, Quest *_Quest, uint32 opt )
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if(!tmpscript || !tmpscript->pGOChooseReward) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOChooseReward(player,_GO,_Quest,opt);
}

MANGOS_DLL_EXPORT
bool AreaTrigger      ( Player *player, AreaTriggerEntry* atEntry )
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[GetAreaTriggerScriptId(atEntry->id)];
    if(!tmpscript || !tmpscript->pAreaTrigger) return false;

    return tmpscript->pAreaTrigger(player, atEntry);
}

MANGOS_DLL_EXPORT
bool ItemUse( Player *player, Item* _Item, SpellCastTargets const& targets)
{
    Script *tmpscript = NULL;

    tmpscript = m_scripts[_Item->GetProto()->ScriptId];
    if(!tmpscript || !tmpscript->pItemUse) return false;

    return tmpscript->pItemUse(player,_Item,targets);
}

MANGOS_DLL_EXPORT
CreatureAI* GetAI(Creature *_Creature )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if(!tmpscript || !tmpscript->GetAI) return NULL;

    return tmpscript->GetAI(_Creature);
}

MANGOS_DLL_EXPORT
InstanceData* CreateInstanceData(Map *map)
{
    if(!map->IsDungeon()) return NULL;
    Script *tmpscript = m_scripts[((InstanceMap*)map)->GetScriptId()];
    if(!tmpscript || !tmpscript->GetInstanceData) return NULL;

    return tmpscript->GetInstanceData(map);
}

MANGOS_DLL_EXPORT
bool EffectDummyGameObj(Unit *caster, uint32 spellId, uint32 effIndex, GameObject *gameObjTarget )
{
    Script *tmpscript = m_scripts[gameObjTarget->GetGOInfo()->ScriptId];

    if (!tmpscript || !tmpscript->pEffectDummyGameObj) return false;

    return tmpscript->pEffectDummyGameObj(caster, spellId,effIndex,gameObjTarget);
}

MANGOS_DLL_EXPORT
bool EffectDummyCreature(Unit *caster, uint32 spellId, uint32 effIndex, Creature *crTarget )
{
    Script *tmpscript = m_scripts[crTarget->GetScriptId()];

    if (!tmpscript || !tmpscript->pEffectDummyCreature) return false;

    return tmpscript->pEffectDummyCreature(caster, spellId,effIndex,crTarget);
}

MANGOS_DLL_EXPORT
bool EffectDummyItem(Unit *caster, uint32 spellId, uint32 effIndex, Item *itemTarget )
{
    Script *tmpscript = m_scripts[itemTarget->GetProto()->ScriptId];

    if (!tmpscript || !tmpscript->pEffectDummyItem) return false;

    return tmpscript->pEffectDummyItem(caster, spellId,effIndex,itemTarget);
}

void ScriptedAI::UpdateAI(const uint32)
{
    //Check if we have a current target
    if( m_creature->isAlive() && m_creature->SelectHostileTarget() && m_creature->getVictim())
    {
        //If we are within range melee the target
        if( m_creature->IsWithinDistInMap(m_creature->getVictim(), ATTACK_DISTANCE))
        {
            if( m_creature->isAttackReady() )
            {
                m_creature->AttackerStateUpdate(m_creature->getVictim());
                m_creature->resetAttackTimer();
            }
        }
    }
}

void ScriptedAI::EnterEvadeMode()
{
    m_creature->CombatStop(true);
    if( m_creature->isAlive() )
        DoGoHome();
}

void ScriptedAI::DoStartAttack(Unit* victim)
{
    if( m_creature->Attack(victim, true) )
        m_creature->GetMotionMaster()->MoveChase(victim);
}

void ScriptedAI::DoStopAttack()
{
    if( m_creature->getVictim() != NULL )
    {
        m_creature->AttackStop();
    }
}

void ScriptedAI::DoGoHome()
{
    if( !m_creature->getVictim() && m_creature->isAlive() )
        m_creature->GetMotionMaster()->MoveTargetedHome();
}
