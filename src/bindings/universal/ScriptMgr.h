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

#ifndef SCRIPTMGR_H
#define SCRIPTMGR_H

//Only required includes
#include "../../game/CreatureAI.h"
#include "../../game/Creature.h"
#include "../../game/InstanceData.h"

class Player;
class Creature;
class Quest;
class Item;
class GameObject;
class SpellCastTargets;
class Map;
class Aura;

#define MAX_SCRIPTS 1000
#define MAX_INSTANCE_SCRIPTS 1000

struct Script
{
    Script() :
        pGossipHello(NULL), pGOGossipHello(NULL), pQuestAccept(NULL), pGossipSelect(NULL), pGOGossipSelect(NULL),
        pGossipSelectWithCode(NULL), pGOGossipSelectWithCode(NULL),
        pQuestSelect(NULL), pQuestComplete(NULL), pNPCDialogStatus(NULL), pGODialogStatus(NULL), pChooseReward(NULL),
        pItemHello(NULL), pGOHello(NULL), pProcessEventId(NULL), pAreaTrigger(NULL), pItemQuestAccept(NULL), pGOQuestAccept(NULL),
        pGOChooseReward(NULL), pItemUse(NULL), pEffectDummyGameObj(NULL), pEffectDummyCreature(NULL),
        pEffectDummyItem(NULL), pEffectAuraDummy(NULL), GetAI(NULL)
    {}

    std::string Name;

    // -- Quest/gossip Methods to be scripted --
    bool (*pGossipHello         )(Player *player, Creature *_Creature);
    bool (*pGOGossipHello       )(Player *player, GameObject *_GO);
    bool (*pQuestAccept         )(Player *player, Creature *_Creature, Quest const*_Quest );
    bool (*pGossipSelect        )(Player *player, Creature *_Creature, uint32 sender, uint32 action );
    bool (*pGOGossipSelect      )(Player *player, GameObject *_GO, uint32 sender, uint32 action );
    bool (*pGossipSelectWithCode)(Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* sCode );
    bool (*pGOGossipSelectWithCode)(Player *player, GameObject *_GO, uint32 sender, uint32 action, const char* sCode );
    bool (*pQuestSelect         )(Player *player, Creature *_Creature, Quest const*_Quest );
    bool (*pQuestComplete       )(Player *player, Creature *_Creature, Quest const*_Quest );
    uint32 (*pNPCDialogStatus   )(Player *player, Creature *_Creature );
    uint32 (*pGODialogStatus    )(Player *player, GameObject * _GO );
    bool (*pChooseReward        )(Player *player, Creature *_Creature, Quest const*_Quest, uint32 opt );
    bool (*pItemHello           )(Player *player, Item *_Item, Quest const*_Quest );
    bool (*pGOHello             )(Player *player, GameObject *_GO );
    bool (*pAreaTrigger         )(Player *player, AreaTriggerEntry const* at);
    bool (*pProcessEventId      )(uint32 eventId, Object* source, Object* target, bool isStart);
    bool (*pItemQuestAccept     )(Player *player, Item *_Item, Quest const*_Quest );
    bool (*pGOQuestAccept       )(Player *player, GameObject *_GO, Quest const*_Quest );
    bool (*pGOChooseReward      )(Player *player, GameObject *_GO, Quest const*_Quest, uint32 opt );
    bool (*pItemUse             )(Player *player, Item* _Item, SpellCastTargets const& targets);
    bool (*pEffectDummyGameObj  )(Unit*, uint32, SpellEffectIndex, GameObject* );
    bool (*pEffectDummyCreature )(Unit*, uint32, SpellEffectIndex, Creature* );
    bool (*pEffectDummyItem     )(Unit*, uint32, SpellEffectIndex, Item* );
    bool (*pEffectAuraDummy     )(const Aura*, bool);

    CreatureAI* (*GetAI)(Creature *_Creature);
    InstanceData* (*GetInstanceData)(Map*);
    // -----------------------------------------

    void registerSelf();
};

#define VISIBLE_RANGE (50.0f)

// Read function descriptions in CreatureAI
struct MANGOS_DLL_DECL ScriptedAI : public CreatureAI
{
    explicit ScriptedAI(Creature* creature) : CreatureAI(creature) {}
    ~ScriptedAI() {}

    // Called at stopping attack by any attacker
    void EnterEvadeMode();

    // Is unit visible for MoveInLineOfSight
    bool IsVisible(Unit* who) const
    {
        return !who->HasStealthAura() && m_creature->IsWithinDist(who,VISIBLE_RANGE);
    }

    // Called at World update tick
    void UpdateAI(const uint32);

    //= Some useful helpers =========================

    // Start attack of victim and go to him
    void DoStartAttack(Unit* victim);

    // Stop attack of current victim
    void DoStopAttack();

    // Cast spell
    void DoCast(Unit* victim, uint32 spelId)
    {
        m_creature->CastSpell(victim,spelId,true);
    }

    void DoCastSpell(Unit* who,SpellEntry *spellInfo)
    {
        m_creature->CastSpell(who,spellInfo,true);
    }

    void DoSay(int32 text_id, uint32 language)
    {
        m_creature->MonsterSay(text_id, language);
    }

    void DoGoHome();
};

#endif
