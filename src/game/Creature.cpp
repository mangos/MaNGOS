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

#include "Creature.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ObjectGuid.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "Player.h"
#include "GameEventMgr.h"
#include "PoolManager.h"
#include "Opcodes.h"
#include "Log.h"
#include "LootMgr.h"
#include "MapManager.h"
#include "CreatureAI.h"
#include "CreatureAISelector.h"
#include "Formulas.h"
#include "WaypointMovementGenerator.h"
#include "InstanceData.h"
#include "BattleGroundMgr.h"
#include "Spell.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

// apply implementation of the singletons
#include "Policies/SingletonImp.h"

TrainerSpell const* TrainerSpellData::Find(uint32 spell_id) const
{
    TrainerSpellMap::const_iterator itr = spellList.find(spell_id);
    if (itr != spellList.end())
        return &itr->second;

    return NULL;
}

bool VendorItemData::RemoveItem( uint32 item_id )
{
    bool found = false;
    for(VendorItemList::iterator i = m_items.begin(); i != m_items.end(); )
    {
        // can have many examples
        if((*i)->item == item_id)
        {
            i = m_items.erase(i);
            found = true;
        }
        else
            ++i;
    }

    return found;
}

VendorItem const* VendorItemData::FindItemCostPair(uint32 item_id, uint32 extendedCost) const
{
    for(VendorItemList::const_iterator i = m_items.begin(); i != m_items.end(); ++i )
        if((*i)->item == item_id && (*i)->ExtendedCost == extendedCost)
            return *i;
    return NULL;
}

bool AssistDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    if (Unit* victim = m_owner.GetMap()->GetUnit(m_victimGuid))
    {
        while (!m_assistantGuids.empty())
        {
            Creature* assistant = m_owner.GetMap()->GetAnyTypeCreature(*m_assistantGuids.rbegin());
            m_assistantGuids.pop_back();

            if (assistant && assistant->CanAssistTo(&m_owner, victim))
            {
                assistant->SetNoCallAssistance(true);
                if(assistant->AI())
                    assistant->AI()->AttackStart(victim);
            }
        }
    }
    return true;
}

AssistDelayEvent::AssistDelayEvent( ObjectGuid victim, Unit& owner, std::list<Creature*> const& assistants ) : BasicEvent(), m_victimGuid(victim), m_owner(owner)
{
    // Pushing guids because in delay can happen some creature gets despawned => invalid pointer
    m_assistantGuids.reserve(assistants.size());
    for (std::list<Creature*>::const_iterator itr = assistants.begin(); itr != assistants.end(); ++itr)
        m_assistantGuids.push_back((*itr)->GetObjectGuid());
}

bool ForcedDespawnDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    m_owner.ForcedDespawn();
    return true;
}

Creature::Creature(CreatureSubtype subtype) :
Unit(), i_AI(NULL),
lootForPickPocketed(false), lootForBody(false), lootForSkin(false),m_lootMoney(0),
m_corpseDecayTimer(0), m_respawnTime(0), m_respawnDelay(25), m_corpseDelay(60), m_respawnradius(5.0f),
m_subtype(subtype), m_defaultMovementType(IDLE_MOTION_TYPE), m_DBTableGuid(0), m_equipmentId(0),
m_AlreadyCallAssistance(false), m_AlreadySearchedAssistance(false),
m_regenHealth(true), m_AI_locked(false), m_isDeadByDefault(false), m_needNotify(false),
m_meleeDamageSchoolMask(SPELL_SCHOOL_MASK_NORMAL),
m_creatureInfo(NULL), m_splineFlags(SPLINEFLAG_WALKMODE)
{
    m_regenTimer = 200;
    m_valuesCount = UNIT_END;

    for(int i = 0; i < CREATURE_MAX_SPELLS; ++i)
        m_spells[i] = 0;

    m_CreatureSpellCooldowns.clear();
    m_CreatureCategoryCooldowns.clear();

    m_splineFlags = SPLINEFLAG_WALKMODE;
}

Creature::~Creature()
{
    CleanupsBeforeDelete();

    m_vendorItemCounts.clear();

    delete i_AI;
    i_AI = NULL;
}

void Creature::AddToWorld()
{
    ///- Register the creature for guid lookup
    if(!IsInWorld() && GetObjectGuid().IsCreatureOrVehicle())
        GetMap()->GetObjectsStore().insert<Creature>(GetGUID(), (Creature*)this);

    Unit::AddToWorld();

    if (GetVehicleKit())
        GetVehicleKit()->Reset();
}

void Creature::RemoveFromWorld()
{
    ///- Remove the creature from the accessor
    if(IsInWorld() && GetObjectGuid().IsCreatureOrVehicle())
        GetMap()->GetObjectsStore().erase<Creature>(GetGUID(), (Creature*)NULL);

    Unit::RemoveFromWorld();
}

void Creature::RemoveCorpse()
{
    if (((getDeathState() != CORPSE && getDeathState() != GHOULED) && !m_isDeadByDefault) || (getDeathState() != ALIVE && m_isDeadByDefault))
        return;

    m_corpseDecayTimer = 0;
    SetDeathState(DEAD);
    UpdateObjectVisibility();

    // stop loot rolling before loot clear and for close client dialogs
    StopGroupLoot();

    loot.clear();
    uint32 respawnDelay = 0;

    if (AI())
        AI()->CorpseRemoved(respawnDelay);

    // script can set time (in seconds) explicit, override the original
    if (respawnDelay)
        m_respawnTime = time(NULL) + respawnDelay;

    float x, y, z, o;
    GetRespawnCoord(x, y, z, &o);
    GetMap()->CreatureRelocation(this, x, y, z, o);
}

/**
 * change the entry of creature until respawn
 */
bool Creature::InitEntry(uint32 Entry, CreatureData const* data /*=NULL*/, GameEventCreatureData const* eventData /*=NULL*/ )
{
    // use game event entry if any instead default suggested
    if (eventData && eventData->entry_id)
        Entry = eventData->entry_id;

    CreatureInfo const *normalInfo = ObjectMgr::GetCreatureTemplate(Entry);
    if(!normalInfo)
    {
        sLog.outErrorDb("Creature::UpdateEntry creature entry %u does not exist.", Entry);
        return false;
    }

    // difficulties for dungeons/battleground ordered in normal way
    // and if more high version not exist must be used lesser version
    // for raid order different:
    // 10 man normal version must be used instead nonexistent 10 man heroic version
    // 25 man normal version must be used instead nonexistent 25 man heroic version
    CreatureInfo const *cinfo = normalInfo;
    for (uint8 diff = uint8(GetMap()->GetDifficulty()); diff > 0;)
    {
        // we already have valid Map pointer for current creature!
        if (normalInfo->DifficultyEntry[diff - 1])
        {
            cinfo = ObjectMgr::GetCreatureTemplate(normalInfo->DifficultyEntry[diff - 1]);
            if (cinfo)
                break;                                      // template found

            // check and reported at startup, so just ignore (restore normalInfo)
            cinfo = normalInfo;
        }

        // for raid heroic to normal, for other to prev in normal order
        if ((diff == int(RAID_DIFFICULTY_10MAN_HEROIC) || diff == int(RAID_DIFFICULTY_25MAN_HEROIC)) &&
            GetMap()->IsRaid())
            diff -= 2;                                      // to normal raid difficulty cases
        else
            --diff;
    }

    SetEntry(Entry);                                        // normal entry always
    m_creatureInfo = cinfo;                                 // map mode related always

    SetObjectScale(cinfo->scale);

    // equal to player Race field, but creature does not have race
    SetByteValue(UNIT_FIELD_BYTES_0, 0, 0);

    // known valid are: CLASS_WARRIOR,CLASS_PALADIN,CLASS_ROGUE,CLASS_MAGE
    SetByteValue(UNIT_FIELD_BYTES_0, 1, uint8(cinfo->unit_class));

    uint32 display_id = ChooseDisplayId(GetCreatureInfo(), data, eventData);
    if (!display_id)                                        // Cancel load if no display id
    {
        sLog.outErrorDb("Creature (Entry: %u) has no model defined in table `creature_template`, can't load.", Entry);
        return false;
    }

    CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
    if (!minfo)                                             // Cancel load if no model defined
    {
        sLog.outErrorDb("Creature (Entry: %u) has no model info defined in table `creature_model_info`, can't load.", Entry);
        return false;
    }

    display_id = minfo->modelid;                            // it can be different (for another gender)

    SetNativeDisplayId(display_id);

    // normally the same as native, but some has exceptions (Spell::DoSummonTotem)
    SetDisplayId(display_id);

    SetByteValue(UNIT_FIELD_BYTES_0, 2, minfo->gender);
    SetByteValue(UNIT_FIELD_BYTES_0, 3, uint8(cinfo->powerType));

    // Load creature equipment
    if (eventData && eventData->equipment_id)
    {
        LoadEquipment(eventData->equipment_id);             // use event equipment if any for active event
    }
    else if (!data || data->equipmentId == 0)
    {
        if (cinfo->equipmentId == 0)
            LoadEquipment(normalInfo->equipmentId);         // use default from normal template if diff does not have any
        else
            LoadEquipment(cinfo->equipmentId);              // else use from diff template
    }
    else if (data && data->equipmentId != -1)
    {                                                       // override, -1 means no equipment
        LoadEquipment(data->equipmentId);
    }

    SetName(normalInfo->Name);                              // at normal entry always

    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);

    // update speed for the new CreatureInfo base speed mods
    UpdateSpeed(MOVE_WALK, false);
    UpdateSpeed(MOVE_RUN,  false);

    // checked at loading
    m_defaultMovementType = MovementGeneratorType(cinfo->MovementType);

    return true;
}

bool Creature::UpdateEntry(uint32 Entry, Team team, const CreatureData *data /*=NULL*/, GameEventCreatureData const* eventData /*=NULL*/, bool preserveHPAndPower /*=true*/)
{
    if (!InitEntry(Entry, data, eventData))
        return false;

    m_regenHealth = GetCreatureInfo()->RegenHealth;

    // creatures always have melee weapon ready if any
    SetSheath(SHEATH_STATE_MELEE);

    SelectLevel(GetCreatureInfo(), preserveHPAndPower ? GetHealthPercent() : 100.0f, 100.0f);

    if (team == HORDE)
        setFaction(GetCreatureInfo()->faction_H);
    else
        setFaction(GetCreatureInfo()->faction_A);

    SetUInt32Value(UNIT_NPC_FLAGS,GetCreatureInfo()->npcflag);

    SetAttackTime(BASE_ATTACK,  GetCreatureInfo()->baseattacktime);
    SetAttackTime(OFF_ATTACK,   GetCreatureInfo()->baseattacktime);
    SetAttackTime(RANGED_ATTACK,GetCreatureInfo()->rangeattacktime);

    uint32 unitFlags = GetCreatureInfo()->unit_flags;

    // we may need to append or remove additional flags
    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT))
        unitFlags |= UNIT_FLAG_IN_COMBAT;

    SetUInt32Value(UNIT_FIELD_FLAGS, unitFlags);

    // preserve all current dynamic flags if exist
    uint32 dynFlags = GetUInt32Value(UNIT_DYNAMIC_FLAGS);
    SetUInt32Value(UNIT_DYNAMIC_FLAGS, dynFlags ? dynFlags : GetCreatureInfo()->dynamicflags);

    SetModifierValue(UNIT_MOD_ARMOR,             BASE_VALUE, float(GetCreatureInfo()->armor));
    SetModifierValue(UNIT_MOD_RESISTANCE_HOLY,   BASE_VALUE, float(GetCreatureInfo()->resistance1));
    SetModifierValue(UNIT_MOD_RESISTANCE_FIRE,   BASE_VALUE, float(GetCreatureInfo()->resistance2));
    SetModifierValue(UNIT_MOD_RESISTANCE_NATURE, BASE_VALUE, float(GetCreatureInfo()->resistance3));
    SetModifierValue(UNIT_MOD_RESISTANCE_FROST,  BASE_VALUE, float(GetCreatureInfo()->resistance4));
    SetModifierValue(UNIT_MOD_RESISTANCE_SHADOW, BASE_VALUE, float(GetCreatureInfo()->resistance5));
    SetModifierValue(UNIT_MOD_RESISTANCE_ARCANE, BASE_VALUE, float(GetCreatureInfo()->resistance6));

    SetCanModifyStats(true);
    UpdateAllStats();

    // checked and error show at loading templates
    if (FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(GetCreatureInfo()->faction_A))
    {
        if (factionTemplate->factionFlags & FACTION_TEMPLATE_FLAG_PVP)
            SetPvP(true);
        else
            SetPvP(false);
    }

    for(int i = 0; i < CREATURE_MAX_SPELLS; ++i)
        m_spells[i] = GetCreatureInfo()->spells[i];

    // if eventData set then event active and need apply spell_start
    if (eventData)
        ApplyGameEventSpells(eventData, true);

    return true;
}

uint32 Creature::ChooseDisplayId(const CreatureInfo *cinfo, const CreatureData *data /*= NULL*/, GameEventCreatureData const* eventData /*=NULL*/)
{
    // Use creature event model explicit, override any other static models
    if (eventData && eventData->modelid)
        return eventData->modelid;

    // Use creature model explicit, override template (creature.modelid)
    if (data && data->modelid_override)
        return data->modelid_override;

    // use defaults from the template
    uint32 display_id = 0;

    // models may be categorized as (in this order):
    // if mod4 && mod3 && mod2 && mod1  use any, by 25%-chance (other gender is selected and replaced after this function)
    // if mod3 && mod2 && mod1          use mod3 unless mod2 has modelid_alt_model (then all by 33%-chance)
    // if mod2                          use mod2 unless mod2 has modelid_alt_model (then both by 50%-chance)
    // if mod1                          use mod1

    // model selected here may be replaced with other_gender using own function

    if (cinfo->ModelId[3] && cinfo->ModelId[2] && cinfo->ModelId[1] && cinfo->ModelId[0])
    {
        display_id = cinfo->ModelId[urand(0,3)];
    }
    else if (cinfo->ModelId[2] && cinfo->ModelId[1] && cinfo->ModelId[0])
    {
        uint32 modelid_tmp = sObjectMgr.GetCreatureModelAlternativeModel(cinfo->ModelId[1]);
        display_id = modelid_tmp ? cinfo->ModelId[urand(0,2)] : cinfo->ModelId[2];
    }
    else if (cinfo->ModelId[1])
    {
        // We use this to eliminate invisible models vs. "dummy" models (infernals, etc).
        // Where it's expected to select one of two, model must have a alternative model defined (alternative model is normally the same as defined in ModelId1).
        // Same pattern is used in the above model selection, but the result may be ModelId3 and not ModelId2 as here.
        uint32 modelid_tmp = sObjectMgr.GetCreatureModelAlternativeModel(cinfo->ModelId[1]);
        display_id = modelid_tmp ? modelid_tmp : cinfo->ModelId[1];
    }
    else if (cinfo->ModelId[0])
    {
        display_id = cinfo->ModelId[0];
    }

    // fail safe, we use creature entry 1 and make error
    if (!display_id)
    {
        sLog.outErrorDb("Call customer support, ChooseDisplayId can not select native model for creature entry %u, model from creature entry 1 will be used instead.", cinfo->Entry);

        if (const CreatureInfo *creatureDefault = ObjectMgr::GetCreatureTemplate(1))
            display_id = creatureDefault->ModelId[0];
    }

    return display_id;
}

void Creature::Update(uint32 update_diff, uint32 diff)
{
    if (m_needNotify)
    {
        m_needNotify = false;
        RelocationNotify();

        if (!IsInWorld())
            return;
    }

    switch( m_deathState )
    {
        case JUST_ALIVED:
            // Don't must be called, see Creature::SetDeathState JUST_ALIVED -> ALIVE promoting.
            sLog.outError("Creature (GUIDLow: %u Entry: %u ) in wrong state: JUST_ALIVED (4)",GetGUIDLow(),GetEntry());
            break;
        case JUST_DIED:
            // Don't must be called, see Creature::SetDeathState JUST_DIED -> CORPSE promoting.
            sLog.outError("Creature (GUIDLow: %u Entry: %u ) in wrong state: JUST_DEAD (1)",GetGUIDLow(),GetEntry());
            break;
        case DEAD:
        {
            if( m_respawnTime <= time(NULL) )
            {
                DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Respawning...");
                m_respawnTime = 0;
                lootForPickPocketed = false;
                lootForBody         = false;
                lootForSkin         = false;

                if(m_originalEntry != GetEntry())
                {
                    // need preserver gameevent state
                    GameEventCreatureData const* eventData = sGameEventMgr.GetCreatureUpdateDataForActiveEvent(GetDBTableGUIDLow());
                    UpdateEntry(m_originalEntry, TEAM_NONE, NULL, eventData);
                }

                CreatureInfo const *cinfo = GetCreatureInfo();

                SelectLevel(cinfo);
                SetUInt32Value(UNIT_DYNAMIC_FLAGS, 0);
                if (m_isDeadByDefault)
                {
                    SetDeathState(JUST_DIED);
                    SetHealth(0);
                    i_motionMaster.Clear();
                    clearUnitState(UNIT_STAT_ALL_STATE);
                    LoadCreatureAddon(true);
                }
                else
                    SetDeathState( JUST_ALIVED );

                //Call AI respawn virtual function
                if (AI())
                    AI()->JustRespawned();

                GetMap()->Add(this);
            }
            break;
        }
        case CORPSE:
        {
            if (m_isDeadByDefault)
                break;

            if (m_corpseDecayTimer <= update_diff)
            {
                // since pool system can fail to roll unspawned object, this one can remain spawned, so must set respawn nevertheless
                uint16 poolid = GetDBTableGUIDLow() ? sPoolMgr.IsPartOfAPool<Creature>(GetDBTableGUIDLow()) : 0;
                if (poolid)
                    sPoolMgr.UpdatePool<Creature>(poolid, GetDBTableGUIDLow());

                if (IsInWorld())                            // can be despawned by update pool
                {
                    RemoveCorpse();
                    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Removing corpse... %u ", GetEntry());
                }
            }
            else
            {
                m_corpseDecayTimer -= update_diff;
                if (m_groupLootId)
                {
                    if(update_diff < m_groupLootTimer)
                        m_groupLootTimer -= update_diff;
                    else
                        StopGroupLoot();
                }
            }

            break;
        }
        case ALIVE:
        {
            if (m_isDeadByDefault)
            {
                if (m_corpseDecayTimer <= update_diff)
                {
                    // since pool system can fail to roll unspawned object, this one can remain spawned, so must set respawn nevertheless
                    uint16 poolid = GetDBTableGUIDLow() ? sPoolMgr.IsPartOfAPool<Creature>(GetDBTableGUIDLow()) : 0;

                    if (poolid)
                        sPoolMgr.UpdatePool<Creature>(poolid, GetDBTableGUIDLow());

                    if (IsInWorld())                        // can be despawned by update pool
                    {
                        RemoveCorpse();
                        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Removing alive corpse... %u ", GetEntry());
                    }
                    else
                        return;
                }
                else
                {
                    m_corpseDecayTimer -= update_diff;
                }
            }

            Unit::Update( update_diff, diff );

            // creature can be dead after Unit::Update call
            // CORPSE/DEAD state will processed at next tick (in other case death timer will be updated unexpectedly)
            if(!isAlive())
                break;

            if(!IsInEvadeMode())
            {
                if (AI())
                {
                    // do not allow the AI to be changed during update
                    m_AI_locked = true;
                    AI()->UpdateAI(diff);   // AI not react good at real update delays (while freeze in non-active part of map)
                    m_AI_locked = false;
                }
            }

            // creature can be dead after UpdateAI call
            // CORPSE/DEAD state will processed at next tick (in other case death timer will be updated unexpectedly)
            if(!isAlive())
                break;

            if (IsPet())                           // Regenerated before
                break;

            if(m_regenTimer > 0)
            {
                if(update_diff >= m_regenTimer)
                    m_regenTimer = 0;
                else
                    m_regenTimer -= update_diff;
            }
            if (m_regenTimer != 0)
                break;

            if (!isInCombat() || IsPolymorphed())
                RegenerateHealth();

            Regenerate(getPowerType());
            m_regenTimer = REGEN_TIME_FULL;

            break;
        }
        case CORPSE_FALLING:
        {
            SetDeathState(CORPSE);
        }
        default:
            break;
    }
}

void Creature::Regenerate(Powers power)
{
    uint32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

    if (curValue >= maxValue)
        return;

    float addvalue = 0.0f;

    switch(power)
    {
        case POWER_MANA:
        {
            // Combat and any controlled creature
            if (isInCombat() || !GetCharmerOrOwnerGuid().IsEmpty())
            {
                if(!IsUnderLastManaUseEffect())
                {
                    float ManaIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_MANA);
                    float Spirit = GetStat(STAT_SPIRIT);

                    addvalue = int32((Spirit / 5.0f + 17.0f) * ManaIncreaseRate);
                }
            }
            else
                addvalue = maxValue / 3;
            break;
        }
        case POWER_ENERGY:
            if (GetObjectGuid().IsVehicle())
            {
                if (VehicleEntry const* vehicleInfo = sVehicleStore.LookupEntry(GetCreatureInfo()->VehicleId))
                {

                    switch (vehicleInfo->m_powerType)
                    {
                        case ENERGY_TYPE_PYRITE:
                        case ENERGY_TYPE_BLOOD:
                        case ENERGY_TYPE_OOZE:
                        break;

                        case ENERGY_TYPE_STEAM:
                        default:
                            addvalue = 10 * sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_ENERGY);
                        break;
                    }
                }
            }
            else
                addvalue = 20 * sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_ENERGY);
            break;
        case POWER_FOCUS:
            addvalue = 24 * sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_FOCUS);
            break;
        default:
            return;
    }

    // Apply modifiers (if any)

    AuraList const& ModPowerRegenAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN);
    for(AuraList::const_iterator i = ModPowerRegenAuras.begin(); i != ModPowerRegenAuras.end(); ++i)
        if ((*i)->GetModifier()->m_miscvalue == power)
            addvalue += (*i)->GetModifier()->m_amount;

    AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
    for(AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
        if ((*i)->GetModifier()->m_miscvalue == power)
            addvalue *= ((*i)->GetModifier()->m_amount + 100) / 100.0f;

    ModifyPower(power, int32(addvalue));
}

void Creature::RegenerateHealth()
{
    if (!IsRegeneratingHealth())
        return;

    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue)
        return;

    uint32 addvalue = 0;

    // Not only pet, but any controlled creature
    if (!GetCharmerOrOwnerGuid().IsEmpty())
    {
        float HealthIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_HEALTH);
        float Spirit = GetStat(STAT_SPIRIT);

        if( GetPower(POWER_MANA) > 0 )
            addvalue = uint32(Spirit * 0.25 * HealthIncreaseRate);
        else
            addvalue = uint32(Spirit * 0.80 * HealthIncreaseRate);
    }
    else
        addvalue = maxValue/3;

    ModifyHealth(addvalue);
}

void Creature::DoFleeToGetAssistance()
{
    if (!getVictim())
        return;

    float radius = sWorld.getConfig(CONFIG_FLOAT_CREATURE_FAMILY_FLEE_ASSISTANCE_RADIUS);
    if (radius >0)
    {
        Creature* pCreature = NULL;

        MaNGOS::NearestAssistCreatureInCreatureRangeCheck u_check(this, getVictim(), radius);
        MaNGOS::CreatureLastSearcher<MaNGOS::NearestAssistCreatureInCreatureRangeCheck> searcher(pCreature, u_check);
        Cell::VisitGridObjects(this, searcher, radius);

        SetNoSearchAssistance(true);
        UpdateSpeed(MOVE_RUN, false);

        if(!pCreature)
            SetFeared(true, getVictim()->GetObjectGuid(), 0 ,sWorld.getConfig(CONFIG_UINT32_CREATURE_FAMILY_FLEE_DELAY));
        else
            GetMotionMaster()->MoveSeekAssistance(pCreature->GetPositionX(), pCreature->GetPositionY(), pCreature->GetPositionZ());
    }
}

bool Creature::AIM_Initialize()
{
    // make sure nothing can change the AI during AI update
    if(m_AI_locked)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "AIM_Initialize: failed to init, locked.");
        return false;
    }

    CreatureAI * oldAI = i_AI;
    i_motionMaster.Initialize();
    i_AI = FactorySelector::selectAI(this);
    if (oldAI)
        delete oldAI;
    return true;
}

bool Creature::Create(uint32 guidlow, Map *map, uint32 phaseMask, uint32 Entry, Team team /*= TEAM_NONE*/, const CreatureData *data /*= NULL*/, GameEventCreatureData const* eventData /*= NULL*/)
{
    CreatureInfo const *cinfo = sObjectMgr.GetCreatureTemplate(Entry);

    if (!cinfo)
    {
        sLog.outErrorDb("Creature entry %u does not exist.", Entry);
        return false;
    }

    MANGOS_ASSERT(map);

    HighGuid hi = cinfo->VehicleId ? HIGHGUID_VEHICLE : HIGHGUID_UNIT;

    if (map->GetInstanceId() == 0)
    {
        // Creature can be loaded already in map if grid has been unloaded while creature walk to another grid
        // FIXME: until creature guids is global and for instances used dynamic generated guids
        // in instance possible load creature duplicates with same DB guid but different in game guids
        // This will be until implementing per-map creature guids
        if (map->GetCreature(ObjectGuid(hi, Entry, guidlow)))
            return false;
    }
    else
        guidlow = sObjectMgr.GenerateLowGuid(hi);

    ObjectGuid guid(hi, Entry, guidlow);

    SetMap(map);
    SetPhaseMask(phaseMask,false);

    //oX = x;     oY = y;    dX = x;    dY = y;    m_moveTime = 0;    m_startMove = 0;
    const bool bResult = CreateFromProto(guid, Entry, team, data, eventData);

    if (bResult)
    {
        //Notify the map's instance data.
        //Only works if you create the object in it, not if it is moves to that map.
        //Normally non-players do not teleport to other maps.
        if(map->IsDungeon() && ((InstanceMap*)map)->GetInstanceData())
            ((InstanceMap*)map)->GetInstanceData()->OnCreatureCreate(this);

        switch (GetCreatureInfo()->rank)
        {
            case CREATURE_ELITE_RARE:
                m_corpseDelay = sWorld.getConfig(CONFIG_UINT32_CORPSE_DECAY_RARE);
                break;
            case CREATURE_ELITE_ELITE:
                m_corpseDelay = sWorld.getConfig(CONFIG_UINT32_CORPSE_DECAY_ELITE);
                break;
            case CREATURE_ELITE_RAREELITE:
                m_corpseDelay = sWorld.getConfig(CONFIG_UINT32_CORPSE_DECAY_RAREELITE);
                break;
            case CREATURE_ELITE_WORLDBOSS:
                m_corpseDelay = sWorld.getConfig(CONFIG_UINT32_CORPSE_DECAY_WORLDBOSS);
                break;
            default:
                m_corpseDelay = sWorld.getConfig(CONFIG_UINT32_CORPSE_DECAY_NORMAL);
                break;
        }
        LoadCreatureAddon();
    }

    return bResult;
}

bool Creature::IsTrainerOf(Player* pPlayer, bool msg) const
{
    if (!isTrainer())
        return false;

    // pet trainers not have spells in fact now
    if (GetCreatureInfo()->trainer_type != TRAINER_TYPE_PETS)
    {
        TrainerSpellData const* cSpells = GetTrainerSpells();
        TrainerSpellData const* tSpells = GetTrainerTemplateSpells();

        // for not pet trainer expected not empty trainer list always
        if ((!cSpells || cSpells->spellList.empty()) && (!tSpells || tSpells->spellList.empty()))
        {
            sLog.outErrorDb("Creature %u (Entry: %u) have UNIT_NPC_FLAG_TRAINER but have empty trainer spell list.",
                GetGUIDLow(),GetEntry());
            return false;
        }
    }

    switch(GetCreatureInfo()->trainer_type)
    {
        case TRAINER_TYPE_CLASS:
            if (pPlayer->getClass() != GetCreatureInfo()->trainer_class)
            {
                if (msg)
                {
                    pPlayer->PlayerTalkClass->ClearMenus();
                    switch(GetCreatureInfo()->trainer_class)
                    {
                        case CLASS_DRUID:  pPlayer->PlayerTalkClass->SendGossipMenu( 4913,GetGUID()); break;
                        case CLASS_HUNTER: pPlayer->PlayerTalkClass->SendGossipMenu(10090,GetGUID()); break;
                        case CLASS_MAGE:   pPlayer->PlayerTalkClass->SendGossipMenu(  328,GetGUID()); break;
                        case CLASS_PALADIN:pPlayer->PlayerTalkClass->SendGossipMenu( 1635,GetGUID()); break;
                        case CLASS_PRIEST: pPlayer->PlayerTalkClass->SendGossipMenu( 4436,GetGUID()); break;
                        case CLASS_ROGUE:  pPlayer->PlayerTalkClass->SendGossipMenu( 4797,GetGUID()); break;
                        case CLASS_SHAMAN: pPlayer->PlayerTalkClass->SendGossipMenu( 5003,GetGUID()); break;
                        case CLASS_WARLOCK:pPlayer->PlayerTalkClass->SendGossipMenu( 5836,GetGUID()); break;
                        case CLASS_WARRIOR:pPlayer->PlayerTalkClass->SendGossipMenu( 4985,GetGUID()); break;
                    }
                }
                return false;
            }
            break;
        case TRAINER_TYPE_PETS:
            if (pPlayer->getClass() != CLASS_HUNTER)
            {
                if (msg)
                {
                    pPlayer->PlayerTalkClass->ClearMenus();
                    pPlayer->PlayerTalkClass->SendGossipMenu(3620, GetGUID());
                }
                return false;
            }
            break;
        case TRAINER_TYPE_MOUNTS:
            if (GetCreatureInfo()->trainer_race && pPlayer->getRace() != GetCreatureInfo()->trainer_race)
            {
                if (msg)
                {
                    pPlayer->PlayerTalkClass->ClearMenus();
                    switch(GetCreatureInfo()->trainer_class)
                    {
                        case RACE_DWARF:        pPlayer->PlayerTalkClass->SendGossipMenu(5865,GetGUID()); break;
                        case RACE_GNOME:        pPlayer->PlayerTalkClass->SendGossipMenu(4881,GetGUID()); break;
                        case RACE_HUMAN:        pPlayer->PlayerTalkClass->SendGossipMenu(5861,GetGUID()); break;
                        case RACE_NIGHTELF:     pPlayer->PlayerTalkClass->SendGossipMenu(5862,GetGUID()); break;
                        case RACE_ORC:          pPlayer->PlayerTalkClass->SendGossipMenu(5863,GetGUID()); break;
                        case RACE_TAUREN:       pPlayer->PlayerTalkClass->SendGossipMenu(5864,GetGUID()); break;
                        case RACE_TROLL:        pPlayer->PlayerTalkClass->SendGossipMenu(5816,GetGUID()); break;
                        case RACE_UNDEAD:       pPlayer->PlayerTalkClass->SendGossipMenu( 624,GetGUID()); break;
                        case RACE_BLOODELF:     pPlayer->PlayerTalkClass->SendGossipMenu(5862,GetGUID()); break;
                        case RACE_DRAENEI:      pPlayer->PlayerTalkClass->SendGossipMenu(5864,GetGUID()); break;
                    }
                }
                return false;
            }
            break;
        case TRAINER_TYPE_TRADESKILLS:
            if (GetCreatureInfo()->trainer_spell && !pPlayer->HasSpell(GetCreatureInfo()->trainer_spell))
            {
                if (msg)
                {
                    pPlayer->PlayerTalkClass->ClearMenus();
                    pPlayer->PlayerTalkClass->SendGossipMenu(11031, GetGUID());
                }
                return false;
            }
            break;
        default:
            return false;                                   // checked and error output at creature_template loading
    }
    return true;
}

bool Creature::CanInteractWithBattleMaster(Player* pPlayer, bool msg) const
{
    if(!isBattleMaster())
        return false;

    BattleGroundTypeId bgTypeId = sBattleGroundMgr.GetBattleMasterBG(GetEntry());
    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
        return false;

    if(!msg)
        return pPlayer->GetBGAccessByLevel(bgTypeId);

    if(!pPlayer->GetBGAccessByLevel(bgTypeId))
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(bgTypeId)
        {
            case BATTLEGROUND_AV:  pPlayer->PlayerTalkClass->SendGossipMenu(7616, GetGUID()); break;
            case BATTLEGROUND_WS:  pPlayer->PlayerTalkClass->SendGossipMenu(7599, GetGUID()); break;
            case BATTLEGROUND_AB:  pPlayer->PlayerTalkClass->SendGossipMenu(7642, GetGUID()); break;
            case BATTLEGROUND_EY:
            case BATTLEGROUND_NA:
            case BATTLEGROUND_BE:
            case BATTLEGROUND_AA:
            case BATTLEGROUND_RL:
            case BATTLEGROUND_SA:
            case BATTLEGROUND_DS:
            case BATTLEGROUND_RV: pPlayer->PlayerTalkClass->SendGossipMenu(10024, GetGUID()); break;
            default: break;
        }
        return false;
    }
    return true;
}

bool Creature::CanTrainAndResetTalentsOf(Player* pPlayer) const
{
    return pPlayer->getLevel() >= 10
        && GetCreatureInfo()->trainer_type == TRAINER_TYPE_CLASS
        && pPlayer->getClass() == GetCreatureInfo()->trainer_class;
}

void Creature::PrepareBodyLootState()
{
    loot.clear();

    // only dead
    if (!isAlive())
    {
        // if have normal loot then prepare it access
        if (!lootForBody)
        {
            // have normal loot
            if (GetCreatureInfo()->maxgold > 0 || GetCreatureInfo()->lootid ||
                // ... or can have skinning after
                (GetCreatureInfo()->SkinLootId && sWorld.getConfig(CONFIG_BOOL_CORPSE_EMPTY_LOOT_SHOW)))
            {
                SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                return;
            }
        }

        lootForBody = true;                                 // pass this loot mode

        // if not have normal loot allow skinning if need
        if (!lootForSkin && GetCreatureInfo()->SkinLootId)
        {
            RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
            return;
        }
    }

    RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
}

/**
 * Set player and group (if player group member) who tap creature
 */
void Creature::SetLootRecipient(Unit *unit)
{
    // set the player whose group should receive the right
    // to loot the creature after it dies
    // should be set to NULL after the loot disappears

    if (!unit)
    {
        m_lootRecipientGuid.Clear();
        m_lootGroupRecipientId = 0;
        RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED);
        RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED_BY_PLAYER);
        return;
    }

    Player* player = unit->GetCharmerOrOwnerPlayerOrPlayerItself();
    if(!player)                                             // normal creature, no player involved
        return;

    // set player for non group case or if group will disbanded
    m_lootRecipientGuid = player->GetObjectGuid();

    // set group for group existing case including if player will leave group at loot time
    if (Group* group = player->GetGroup())
        m_lootGroupRecipientId = group->GetId();

    SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED);
    SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED_BY_PLAYER);
}

void Creature::SaveToDB()
{
    // this should only be used when the creature has already been loaded
    // preferably after adding to map, because mapid may not be valid otherwise
    CreatureData const *data = sObjectMgr.GetCreatureData(m_DBTableGuid);
    if(!data)
    {
        sLog.outError("Creature::SaveToDB failed, cannot get creature data!");
        return;
    }

    SaveToDB(GetMapId(), data->spawnMask,GetPhaseMask());
}

void Creature::SaveToDB(uint32 mapid, uint8 spawnMask, uint32 phaseMask)
{
    // update in loaded data
    if (!m_DBTableGuid)
        m_DBTableGuid = GetGUIDLow();
    CreatureData& data = sObjectMgr.NewOrExistCreatureData(m_DBTableGuid);

    uint32 displayId = GetNativeDisplayId();

    // check if it's a custom model and if not, use 0 for displayId
    CreatureInfo const *cinfo = GetCreatureInfo();
    if (cinfo)
    {
        if (displayId != cinfo->ModelId[0] && displayId != cinfo->ModelId[1] &&
            displayId != cinfo->ModelId[2] && displayId != cinfo->ModelId[3])
        {
            for(int i = 0; i < MAX_CREATURE_MODEL && displayId; ++i)
                if (cinfo->ModelId[i])
                    if (CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelInfo(cinfo->ModelId[i]))
                        if (displayId == minfo->modelid_other_gender)
                            displayId = 0;
        }
        else
            displayId = 0;
    }

    // data->guid = guid don't must be update at save
    data.id = GetEntry();
    data.mapid = mapid;
    data.phaseMask = phaseMask;
    data.modelid_override = displayId;
    data.equipmentId = GetEquipmentId();
    data.posX = GetPositionX();
    data.posY = GetPositionY();
    data.posZ = GetPositionZ();
    data.orientation = GetOrientation();
    data.spawntimesecs = m_respawnDelay;
    // prevent add data integrity problems
    data.spawndist = GetDefaultMovementType()==IDLE_MOTION_TYPE ? 0 : m_respawnradius;
    data.currentwaypoint = 0;
    data.curhealth = GetHealth();
    data.curmana = GetPower(POWER_MANA);
    data.is_dead = m_isDeadByDefault;
    // prevent add data integrity problems
    data.movementType = !m_respawnradius && GetDefaultMovementType()==RANDOM_MOTION_TYPE
        ? IDLE_MOTION_TYPE : GetDefaultMovementType();
    data.spawnMask = spawnMask;

    // updated in DB
    WorldDatabase.BeginTransaction();

    WorldDatabase.PExecuteLog("DELETE FROM creature WHERE guid = '%u'", m_DBTableGuid);

    std::ostringstream ss;
    ss << "INSERT INTO creature VALUES ("
        << m_DBTableGuid << ","
        << GetEntry() << ","
        << mapid <<","
        << uint32(spawnMask) << ","                         // cast to prevent save as symbol
        << uint16(GetPhaseMask()) << ","                    // prevent out of range error
        << displayId <<","
        << GetEquipmentId() <<","
        << GetPositionX() << ","
        << GetPositionY() << ","
        << GetPositionZ() << ","
        << GetOrientation() << ","
        << m_respawnDelay << ","                            //respawn time
        << (float) m_respawnradius << ","                   //spawn distance (float)
        << (uint32) (0) << ","                              //currentwaypoint
        << GetHealth() << ","                               //curhealth
        << GetPower(POWER_MANA) << ","                      //curmana
        << (m_isDeadByDefault ? 1 : 0) << ","               //is_dead
        << GetDefaultMovementType() << ")";                 //default movement generator type

    WorldDatabase.PExecuteLog("%s", ss.str().c_str());

    WorldDatabase.CommitTransaction();
}

void Creature::SelectLevel(const CreatureInfo *cinfo, float percentHealth, float percentMana)
{
    uint32 rank = IsPet()? 0 : cinfo->rank;

    // level
    uint32 minlevel = std::min(cinfo->maxlevel, cinfo->minlevel);
    uint32 maxlevel = std::max(cinfo->maxlevel, cinfo->minlevel);
    uint32 level = minlevel == maxlevel ? minlevel : urand(minlevel, maxlevel);
    SetLevel(level);

    float rellevel = maxlevel == minlevel ? 0 : (float(level - minlevel))/(maxlevel - minlevel);

    // health
    float healthmod = _GetHealthMod(rank);

    uint32 minhealth = std::min(cinfo->maxhealth, cinfo->minhealth);
    uint32 maxhealth = std::max(cinfo->maxhealth, cinfo->minhealth);
    uint32 health = uint32(healthmod * (minhealth + uint32(rellevel*(maxhealth - minhealth))));

    SetCreateHealth(health);
    SetMaxHealth(health);

    if (percentHealth == 100.0f)
        SetHealth(health);
    else
        SetHealthPercent(percentHealth);

    SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, float(health));

    Powers powerType = Powers(cinfo->powerType);
    uint32 maxPower = 0;

    switch(powerType)
    {
        case POWER_MANA:
        {
            uint32 minmana = std::min(cinfo->maxmana, cinfo->minmana);
            uint32 maxmana = std::max(cinfo->maxmana, cinfo->minmana);
            maxPower = minmana + uint32(rellevel * (maxmana - minmana));

            SetCreateMana(maxPower);
            break;
        }
        case POWER_ENERGY:
        {
            maxPower = uint32(GetCreatePowers(powerType) * cinfo->power_mod);
            break;
        }
    }

    SetMaxPower(powerType, maxPower);
    SetPower(powerType, maxPower);

    SetModifierValue(UnitMods(UNIT_MOD_POWER_START + powerType), BASE_VALUE, float(maxPower));

    // damage
    float damagemod = _GetDamageMod(rank);

    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, cinfo->mindmg * damagemod);
    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, cinfo->maxdmg * damagemod);

    SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE,cinfo->minrangedmg * damagemod);
    SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE,cinfo->maxrangedmg * damagemod);

    SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, cinfo->attackpower * damagemod);
}

float Creature::_GetHealthMod(int32 Rank)
{
    switch (Rank)                                           // define rates for each elite rank
    {
        case CREATURE_ELITE_NORMAL:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_NORMAL_HP);
        case CREATURE_ELITE_ELITE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_HP);
        case CREATURE_ELITE_RAREELITE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_RAREELITE_HP);
        case CREATURE_ELITE_WORLDBOSS:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_WORLDBOSS_HP);
        case CREATURE_ELITE_RARE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_RARE_HP);
        default:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_HP);
    }
}

float Creature::_GetDamageMod(int32 Rank)
{
    switch (Rank)                                           // define rates for each elite rank
    {
        case CREATURE_ELITE_NORMAL:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_NORMAL_DAMAGE);
        case CREATURE_ELITE_ELITE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_DAMAGE);
        case CREATURE_ELITE_RAREELITE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_RAREELITE_DAMAGE);
        case CREATURE_ELITE_WORLDBOSS:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_WORLDBOSS_DAMAGE);
        case CREATURE_ELITE_RARE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_RARE_DAMAGE);
        default:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_DAMAGE);
    }
}

float Creature::GetSpellDamageMod(int32 Rank)
{
    switch (Rank)                                           // define rates for each elite rank
    {
        case CREATURE_ELITE_NORMAL:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_NORMAL_SPELLDAMAGE);
        case CREATURE_ELITE_ELITE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_SPELLDAMAGE);
        case CREATURE_ELITE_RAREELITE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_RAREELITE_SPELLDAMAGE);
        case CREATURE_ELITE_WORLDBOSS:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_WORLDBOSS_SPELLDAMAGE);
        case CREATURE_ELITE_RARE:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_RARE_SPELLDAMAGE);
        default:
            return sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_SPELLDAMAGE);
    }
}

bool Creature::CreateFromProto(ObjectGuid guid, uint32 Entry, Team team, const CreatureData *data /*=NULL*/, GameEventCreatureData const* eventData /*=NULL*/)
{
    m_originalEntry = Entry;

    Object::_Create(guid);

    if (!UpdateEntry(Entry, team, data, eventData, false))
        return false;

    // Checked at startup
    if (GetCreatureInfo()->VehicleId)
        CreateVehicleKit(GetCreatureInfo()->VehicleId);

    return true;
}

bool Creature::LoadFromDB(uint32 guidlow, Map *map)
{
    CreatureData const* data = sObjectMgr.GetCreatureData(guidlow);

    if(!data)
    {
        sLog.outErrorDb("Creature (GUID: %u) not found in table `creature`, can't load. ", guidlow);
        return false;
    }

    GameEventCreatureData const* eventData = sGameEventMgr.GetCreatureUpdateDataForActiveEvent(guidlow);

    m_DBTableGuid = guidlow;

    if (!Create(guidlow, map, data->phaseMask, data->id, TEAM_NONE, data, eventData))
        return false;

    Relocate(data->posX, data->posY, data->posZ, data->orientation);

    if(!IsPositionValid())
    {
        sLog.outError("Creature (guidlow %d, entry %d) not loaded. Suggested coordinates isn't valid (X: %f Y: %f)", GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        return false;
    }

    m_respawnradius = data->spawndist;

    m_respawnDelay = data->spawntimesecs;
    m_isDeadByDefault = data->is_dead;
    m_deathState = m_isDeadByDefault ? DEAD : ALIVE;

    m_respawnTime  = sObjectMgr.GetCreatureRespawnTime(m_DBTableGuid, GetInstanceId());
    if(m_respawnTime > time(NULL))                          // not ready to respawn
    {
        m_deathState = DEAD;
        if(CanFly())
        {
            float tz = GetTerrain()->GetHeight(data->posX, data->posY, data->posZ, false);
            if(data->posZ - tz > 0.1)
                Relocate(data->posX, data->posY, tz);
        }
    }
    else if(m_respawnTime)                                  // respawn time set but expired
    {
        m_respawnTime = 0;
        sObjectMgr.SaveCreatureRespawnTime(m_DBTableGuid,GetInstanceId(),0);
    }

    uint32 curhealth = data->curhealth;
    if(curhealth)
    {
        curhealth = uint32(curhealth*_GetHealthMod(GetCreatureInfo()->rank));
        if(curhealth < 1)
            curhealth = 1;
    }

    SetHealth(m_deathState == ALIVE ? curhealth : 0);
    SetPower(POWER_MANA, data->curmana);

    SetMeleeDamageSchool(SpellSchools(GetCreatureInfo()->dmgschool));

    // checked at creature_template loading
    m_defaultMovementType = MovementGeneratorType(data->movementType);

    AIM_Initialize();
    return true;
}

void Creature::LoadEquipment(uint32 equip_entry, bool force)
{
    if(equip_entry == 0)
    {
        if (force)
        {
            for (uint8 i = 0; i < 3; ++i)
                SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + i, 0);
            m_equipmentId = 0;
        }
        return;
    }

    EquipmentInfo const *einfo = sObjectMgr.GetEquipmentInfo(equip_entry);
    if (!einfo)
        return;

    m_equipmentId = equip_entry;
    for (uint8 i = 0; i < 3; ++i)
        SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + i, einfo->equipentry[i]);
}

bool Creature::HasQuest(uint32 quest_id) const
{
    QuestRelationsMapBounds bounds = sObjectMgr.GetCreatureQuestRelationsMapBounds(GetEntry());
    for(QuestRelationsMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        if (itr->second == quest_id)
            return true;
    }
    return false;
}

bool Creature::HasInvolvedQuest(uint32 quest_id) const
{
    QuestRelationsMapBounds bounds = sObjectMgr.GetCreatureQuestInvolvedRelationsMapBounds(GetEntry());
    for(QuestRelationsMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        if (itr->second == quest_id)
            return true;
    }
    return false;
}

void Creature::DeleteFromDB()
{
    if (!m_DBTableGuid)
    {
        DEBUG_LOG("Trying to delete not saved creature!");
        return;
    }

    sObjectMgr.SaveCreatureRespawnTime(m_DBTableGuid,GetInstanceId(),0);
    sObjectMgr.DeleteCreatureData(m_DBTableGuid);

    WorldDatabase.BeginTransaction();
    WorldDatabase.PExecuteLog("DELETE FROM creature WHERE guid = '%u'", m_DBTableGuid);
    WorldDatabase.PExecuteLog("DELETE FROM creature_addon WHERE guid = '%u'", m_DBTableGuid);
    WorldDatabase.PExecuteLog("DELETE FROM creature_movement WHERE id = '%u'", m_DBTableGuid);
    WorldDatabase.PExecuteLog("DELETE FROM game_event_creature WHERE guid = '%u'", m_DBTableGuid);
    WorldDatabase.PExecuteLog("DELETE FROM game_event_creature_data WHERE guid = '%u'", m_DBTableGuid);
    WorldDatabase.PExecuteLog("DELETE FROM creature_battleground WHERE guid = '%u'", m_DBTableGuid);
    WorldDatabase.CommitTransaction();
}

float Creature::GetAttackDistance(Unit const* pl) const
{
    float aggroRate = sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO);
    if(aggroRate == 0)
        return 0.0f;

    uint32 playerlevel   = pl->GetLevelForTarget(this);
    uint32 creaturelevel = GetLevelForTarget(pl);

    int32 leveldif       = int32(playerlevel) - int32(creaturelevel);

    // "The maximum Aggro Radius has a cap of 25 levels under. Example: A level 30 char has the same Aggro Radius of a level 5 char on a level 60 mob."
    if ( leveldif < - 25)
        leveldif = -25;

    // "The aggro radius of a mob having the same level as the player is roughly 20 yards"
    float RetDistance = 20;

    // "Aggro Radius varies with level difference at a rate of roughly 1 yard/level"
    // radius grow if playlevel < creaturelevel
    RetDistance -= (float)leveldif;

    if(creaturelevel+5 <= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        // detect range auras
        RetDistance += GetTotalAuraModifier(SPELL_AURA_MOD_DETECT_RANGE);

        // detected range auras
        RetDistance += pl->GetTotalAuraModifier(SPELL_AURA_MOD_DETECTED_RANGE);
    }

    // "Minimum Aggro Radius for a mob seems to be combat range (5 yards)"
    if(RetDistance < 5)
        RetDistance = 5;

    return (RetDistance*aggroRate);
}

void Creature::SetDeathState(DeathState s)
{
    if ((s == JUST_DIED && !m_isDeadByDefault) || (s == JUST_ALIVED && m_isDeadByDefault))
    {
        m_corpseDecayTimer = m_corpseDelay*IN_MILLISECONDS; // the max/default time for corpse decay (before creature is looted/AllLootRemovedFromCorpse() is called)
        m_respawnTime = time(NULL) + m_respawnDelay;        // respawn delay (spawntimesecs)

        // always save boss respawn time at death to prevent crash cheating
        if (sWorld.getConfig(CONFIG_BOOL_SAVE_RESPAWN_TIME_IMMEDIATELY) || IsWorldBoss())
            SaveRespawnTime();
    }

    Unit::SetDeathState(s);

    if (s == JUST_DIED)
    {
        SetTargetGuid(ObjectGuid());                        // remove target selection in any cases (can be set at aura remove in Unit::SetDeathState)
        SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

        if (HasSearchedAssistance())
        {
            SetNoSearchAssistance(false);
            UpdateSpeed(MOVE_RUN, false);
        }

        // return, since we promote to CORPSE_FALLING. CORPSE_FALLING is promoted to CORPSE at next update.
        if (CanFly() && FallGround())
            return;

        Unit::SetDeathState(CORPSE);
    }

    if (s == JUST_ALIVED)
    {
        SetHealth(GetMaxHealth());
        SetLootRecipient(NULL);
        CreatureInfo const *cinfo = GetCreatureInfo();
        SetUInt32Value(UNIT_DYNAMIC_FLAGS, 0);
        RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
        AddSplineFlag(SPLINEFLAG_WALKMODE);
        SetUInt32Value(UNIT_NPC_FLAGS, cinfo->npcflag);
        Unit::SetDeathState(ALIVE);
        clearUnitState(UNIT_STAT_ALL_STATE);
        i_motionMaster.Clear();
        SetMeleeDamageSchool(SpellSchools(cinfo->dmgschool));
        LoadCreatureAddon(true);
    }
}

bool Creature::FallGround()
{
    // Only if state is JUST_DIED. CORPSE_FALLING is set below and promoted to CORPSE later
    if (getDeathState() != JUST_DIED)
        return false;

    // use larger distance for vmap height search than in most other cases
    float tz = GetTerrain()->GetHeight(GetPositionX(), GetPositionY(), GetPositionZ(), true, MAX_FALL_DISTANCE);

    if (tz < INVALID_HEIGHT)
    {
        DEBUG_LOG("FallGround: creature %u at map %u (x: %f, y: %f, z: %f), not able to retrive a proper GetHeight (z: %f).",
            GetEntry(), GetMap()->GetId(), GetPositionX(), GetPositionX(), GetPositionZ(), tz);
    }

    // Abort too if the ground is very near
    if (fabs(GetPositionZ() - tz) < 0.1f)
        return false;

    Unit::SetDeathState(CORPSE_FALLING);

    float dz = tz - GetPositionZ();
    float distance = sqrt(dz*dz);

    // default run speed * 2 explicit, not verified though but result looks proper
    double speed = baseMoveSpeed[MOVE_RUN] * 2;

    speed *= 0.001;                                         // to milliseconds

    uint32 travelTime = uint32(distance/speed);

    DEBUG_LOG("FallGround: traveltime: %u, distance: %f, speed: %f, from %f to %f", travelTime, distance, speed, GetPositionZ(), tz);

    // For creatures that are moving towards target and dies, the visual effect is not nice.
    // It is possibly caused by a xyz mismatch in DestinationHolder's GetLocationNow and the location
    // of the mob in client. For mob that are already reached target or dies while not moving
    // the visual appear to be fairly close to the expected.

    GetMap()->CreatureRelocation(this, GetPositionX(), GetPositionY(), tz, GetOrientation());
    SendMonsterMove(GetPositionX(), GetPositionY(), tz, SPLINETYPE_NORMAL, SPLINEFLAG_FALLING, travelTime);
    return true;
}

void Creature::Respawn()
{
    RemoveCorpse();

    // forced recreate creature object at clients
    UnitVisibility currentVis = GetVisibility();
    SetVisibility(VISIBILITY_RESPAWN);
    UpdateObjectVisibility();
    SetVisibility(currentVis);                              // restore visibility state
    UpdateObjectVisibility();

    if (IsDespawned())
    {
        if (m_DBTableGuid)
            sObjectMgr.SaveCreatureRespawnTime(m_DBTableGuid,GetInstanceId(), 0);
        m_respawnTime = time(NULL);                         // respawn at next tick
    }
}

void Creature::ForcedDespawn(uint32 timeMSToDespawn)
{
    if (timeMSToDespawn)
    {
        ForcedDespawnDelayEvent *pEvent = new ForcedDespawnDelayEvent(*this);

        m_Events.AddEvent(pEvent, m_Events.CalculateTime(timeMSToDespawn));
        return;
    }

    if (isAlive())
        SetDeathState(JUST_DIED);

    RemoveCorpse();
    SetHealth(0);                                           // just for nice GM-mode view
}

bool Creature::IsImmuneToSpell(SpellEntry const* spellInfo)
{
    if (!spellInfo)
        return false;

    if (GetCreatureInfo()->MechanicImmuneMask & (1 << (spellInfo->Mechanic - 1)))
        return true;

    return Unit::IsImmuneToSpell(spellInfo);
}

bool Creature::IsImmuneToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index) const
{
    if (GetCreatureInfo()->MechanicImmuneMask & (1 << (spellInfo->EffectMechanic[index] - 1)))
        return true;

    // Taunt immunity special flag check
    if (GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NOT_TAUNTABLE)
    {
        // Taunt aura apply check
        if (spellInfo->Effect[index] == SPELL_EFFECT_APPLY_AURA)
        {
            if (spellInfo->EffectApplyAuraName[index] == SPELL_AURA_MOD_TAUNT)
                return true;
        }
        // Spell effect taunt check
        else if (spellInfo->Effect[index] == SPELL_EFFECT_ATTACK_ME)
            return true;
    }

    return Unit::IsImmuneToSpellEffect(spellInfo, index);
}

SpellEntry const *Creature::ReachWithSpellAttack(Unit *pVictim)
{
    if(!pVictim)
        return NULL;

    for(uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
    {
        if(!m_spells[i])
            continue;
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(m_spells[i] );
        if(!spellInfo)
        {
            sLog.outError("WORLD: unknown spell id %i", m_spells[i]);
            continue;
        }

        bool bcontinue = true;
        for(int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if( (spellInfo->Effect[j] == SPELL_EFFECT_SCHOOL_DAMAGE )       ||
                (spellInfo->Effect[j] == SPELL_EFFECT_INSTAKILL)            ||
                (spellInfo->Effect[j] == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE) ||
                (spellInfo->Effect[j] == SPELL_EFFECT_HEALTH_LEECH )
                )
            {
                bcontinue = false;
                break;
            }
        }
        if(bcontinue) continue;

        if(spellInfo->manaCost > GetPower(POWER_MANA))
            continue;
        SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(spellInfo->rangeIndex);
        float range = GetSpellMaxRange(srange);
        float minrange = GetSpellMinRange(srange);

        float dist = GetCombatDistance(pVictim);

        //if(!isInFront( pVictim, range ) && spellInfo->AttributesEx )
        //    continue;
        if( dist > range || dist < minrange )
            continue;
        if(spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            continue;
        if(spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
            continue;
        return spellInfo;
    }
    return NULL;
}

SpellEntry const *Creature::ReachWithSpellCure(Unit *pVictim)
{
    if(!pVictim)
        return NULL;

    for(uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
    {
        if(!m_spells[i])
            continue;
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(m_spells[i] );
        if(!spellInfo)
        {
            sLog.outError("WORLD: unknown spell id %i", m_spells[i]);
            continue;
        }

        bool bcontinue = true;
        for(int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if( (spellInfo->Effect[j] == SPELL_EFFECT_HEAL ) )
            {
                bcontinue = false;
                break;
            }
        }
        if(bcontinue)
            continue;

        if(spellInfo->manaCost > GetPower(POWER_MANA))
            continue;
        SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(spellInfo->rangeIndex);
        float range = GetSpellMaxRange(srange);
        float minrange = GetSpellMinRange(srange);

        float dist = GetCombatDistance(pVictim);

        //if(!isInFront( pVictim, range ) && spellInfo->AttributesEx )
        //    continue;
        if( dist > range || dist < minrange )
            continue;
        if(spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            continue;
        if(spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
            continue;
        return spellInfo;
    }
    return NULL;
}

bool Creature::IsVisibleInGridForPlayer(Player* pl) const
{
    // gamemaster in GM mode see all, including ghosts
    if(pl->isGameMaster())
        return true;

    if (GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_INVISIBLE)
        return false;

    // Live player (or with not release body see live creatures or death creatures with corpse disappearing time > 0
    if(pl->isAlive() || pl->GetDeathTimer() > 0)
    {
        return (isAlive() || m_corpseDecayTimer > 0 || (m_isDeadByDefault && m_deathState == CORPSE));
    }

    // Dead player see live creatures near own corpse
    if(isAlive())
    {
        Corpse *corpse = pl->GetCorpse();
        if(corpse)
        {
            // 20 - aggro distance for same level, 25 - max additional distance if player level less that creature level
            if(corpse->IsWithinDistInMap(this,(20+25)*sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO)))
                return true;
        }
    }

    // Dead player can see ghosts
    if (GetCreatureInfo()->type_flags & CREATURE_TYPEFLAGS_GHOST_VISIBLE)
        return true;

    // and not see any other
    return false;
}

void Creature::SendAIReaction(AiReaction reactionType)
{
    WorldPacket data(SMSG_AI_REACTION, 12);

    data << GetObjectGuid();
    data << uint32(reactionType);

    ((WorldObject*)this)->SendMessageToSet(&data, true);

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "WORLD: Sent SMSG_AI_REACTION, type %u.", reactionType);
}

void Creature::CallAssistance()
{
    if( !m_AlreadyCallAssistance && getVictim() && !IsPet() && !isCharmed())
    {
        SetNoCallAssistance(true);

        float radius = sWorld.getConfig(CONFIG_FLOAT_CREATURE_FAMILY_ASSISTANCE_RADIUS);
        if(radius > 0)
        {
            std::list<Creature*> assistList;

            {
                MaNGOS::AnyAssistCreatureInRangeCheck u_check(this, getVictim(), radius);
                MaNGOS::CreatureListSearcher<MaNGOS::AnyAssistCreatureInRangeCheck> searcher(assistList, u_check);
                Cell::VisitGridObjects(this,searcher, radius);
            }

            if (!assistList.empty())
            {
                AssistDelayEvent *e = new AssistDelayEvent(getVictim()->GetObjectGuid(), *this, assistList);
                m_Events.AddEvent(e, m_Events.CalculateTime(sWorld.getConfig(CONFIG_UINT32_CREATURE_FAMILY_ASSISTANCE_DELAY)));
            }
        }
    }
}

void Creature::CallForHelp(float fRadius)
{
    if (fRadius <= 0.0f || !getVictim() || IsPet() || isCharmed())
        return;

    MaNGOS::CallOfHelpCreatureInRangeDo u_do(this, getVictim(), fRadius);
    MaNGOS::CreatureWorker<MaNGOS::CallOfHelpCreatureInRangeDo> worker(this, u_do);
    Cell::VisitGridObjects(this,worker, fRadius);
}

bool Creature::CanAssistTo(const Unit* u, const Unit* enemy, bool checkfaction /*= true*/) const
{
    // we don't need help from zombies :)
    if (!isAlive())
        return false;

    // we don't need help from non-combatant ;)
    if (IsCivilian())
        return false;

    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PASSIVE))
        return false;

    // skip fighting creature
    if (isInCombat())
        return false;

    // only free creature
    if (!GetCharmerOrOwnerGuid().IsEmpty())
        return false;

    // only from same creature faction
    if (checkfaction)
    {
        if (getFaction() != u->getFaction())
            return false;
    }
    else
    {
        if (!IsFriendlyTo(u))
            return false;
    }

    // skip non hostile to caster enemy creatures
    if (!IsHostileTo(enemy))
        return false;

    return true;
}

bool Creature::CanInitiateAttack()
{
    if (hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_DIED))
        return false;

    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE))
        return false;

    if (isPassiveToHostile())
        return false;

    return true;
}

void Creature::SaveRespawnTime()
{
    if(IsPet() || !m_DBTableGuid)
        return;

    if(m_respawnTime > time(NULL))                          // dead (no corpse)
        sObjectMgr.SaveCreatureRespawnTime(m_DBTableGuid, GetInstanceId(), m_respawnTime);
    else if (m_corpseDecayTimer > 0)                        // dead (corpse)
        sObjectMgr.SaveCreatureRespawnTime(m_DBTableGuid, GetInstanceId(), time(NULL) + m_respawnDelay + m_corpseDecayTimer / IN_MILLISECONDS);
}

bool Creature::IsOutOfThreatArea(Unit* pVictim) const
{
    if (!pVictim)
        return true;

    if (!pVictim->IsInMap(this))
        return true;

    if (!pVictim->isTargetableForAttack())
        return true;

    if (!pVictim->isInAccessablePlaceFor(this))
        return true;

    if (!pVictim->isVisibleForOrDetect(this,this,false))
        return true;

    if(sMapStore.LookupEntry(GetMapId())->IsDungeon())
        return false;

    float AttackDist = GetAttackDistance(pVictim);
    float ThreatRadius = sWorld.getConfig(CONFIG_FLOAT_THREAT_RADIUS);

    //Use AttackDistance in distance check if threat radius is lower. This prevents creature bounce in and out of combat every update tick.
    return !pVictim->IsWithinDist3d(CombatStartX, CombatStartY, CombatStartZ,
        ThreatRadius > AttackDist ? ThreatRadius : AttackDist);
}

CreatureDataAddon const* Creature::GetCreatureAddon() const
{
    if (m_DBTableGuid)
    {
        if(CreatureDataAddon const* addon = ObjectMgr::GetCreatureAddon(m_DBTableGuid))
            return addon;
    }

    // dependent from difficulty mode entry
    return ObjectMgr::GetCreatureTemplateAddon(GetCreatureInfo()->Entry);
}

//creature_addon table
bool Creature::LoadCreatureAddon(bool reload)
{
    CreatureDataAddon const *cainfo = GetCreatureAddon();
    if(!cainfo)
        return false;

    if (cainfo->mount != 0)
        Mount(cainfo->mount);

    if (cainfo->bytes1 != 0)
    {
        // 0 StandState
        // 1 FreeTalentPoints   Pet only, so always 0 for default creature
        // 2 StandFlags
        // 3 StandMiscFlags

        SetByteValue(UNIT_FIELD_BYTES_1, 0, uint8(cainfo->bytes1 & 0xFF));
        //SetByteValue(UNIT_FIELD_BYTES_1, 1, uint8((cainfo->bytes1 >> 8) & 0xFF));
        SetByteValue(UNIT_FIELD_BYTES_1, 1, 0);
        SetByteValue(UNIT_FIELD_BYTES_1, 2, uint8((cainfo->bytes1 >> 16) & 0xFF));
        SetByteValue(UNIT_FIELD_BYTES_1, 3, uint8((cainfo->bytes1 >> 24) & 0xFF));
    }

    // UNIT_FIELD_BYTES_2
    // 0 SheathState
    // 1 UnitPVPStateFlags  Set at Creature::UpdateEntry (SetPvp())
    // 2 UnitRename         Pet only, so always 0 for default creature
    // 3 ShapeshiftForm     Must be determined/set by shapeshift spell/aura
    if (cainfo->sheath_state != 0)
        SetByteValue(UNIT_FIELD_BYTES_2, 0, cainfo->sheath_state);

    if (cainfo->pvp_state != 0)
        SetByteValue(UNIT_FIELD_BYTES_2, 1, cainfo->pvp_state);

    //SetByteValue(UNIT_FIELD_BYTES_2, 2, 0);
    //SetByteValue(UNIT_FIELD_BYTES_2, 3, 0);

    if (cainfo->emote != 0)
        SetUInt32Value(UNIT_NPC_EMOTESTATE, cainfo->emote);

    if (cainfo->splineFlags != 0)
        SetSplineFlags(SplineFlags(cainfo->splineFlags));

    if(cainfo->auras)
    {
        for (CreatureDataAddonAura const* cAura = cainfo->auras; cAura->spell_id; ++cAura)
        {
            SpellEntry const *AdditionalSpellInfo = sSpellStore.LookupEntry(cAura->spell_id);
            if (!AdditionalSpellInfo)
            {
                sLog.outErrorDb("Creature (GUIDLow: %u Entry: %u ) has wrong spell %u defined in `auras` field.",GetGUIDLow(),GetEntry(),cAura->spell_id);
                continue;
            }

            // skip already applied aura
            if(HasAura(cAura->spell_id,cAura->effect_idx))
            {
                if(!reload)
                    sLog.outErrorDb("Creature (GUIDLow: %u Entry: %u ) has duplicate aura (spell %u effect %u) in `auras` field.",GetGUIDLow(),GetEntry(),cAura->spell_id,cAura->effect_idx);

                continue;
            }

            SpellAuraHolder *holder = GetSpellAuraHolder(cAura->spell_id, GetGUID());

            bool addedToExisting = true;
            if (!holder)
            {
                holder = CreateSpellAuraHolder(AdditionalSpellInfo, this, this);
                addedToExisting = false;
            }
            Aura* AdditionalAura = CreateAura(AdditionalSpellInfo, cAura->effect_idx, NULL, holder, this, this, 0);
            holder->AddAura(AdditionalAura, cAura->effect_idx);

            if (addedToExisting)
            {
                AddAuraToModList(AdditionalAura);
                holder->SetInUse(true);
                AdditionalAura->ApplyModifier(true,true);
                holder->SetInUse(false);
            }
            else
                AddSpellAuraHolder(holder);

            DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell: %u - Aura %u added to creature (GUIDLow: %u Entry: %u )", cAura->spell_id, AdditionalSpellInfo->EffectApplyAuraName[EFFECT_INDEX_0],GetGUIDLow(),GetEntry());
        }
    }
    return true;
}

/// Send a message to LocalDefense channel for players opposition team in the zone
void Creature::SendZoneUnderAttackMessage(Player* attacker)
{
    Team enemy_team = attacker->GetTeam();

    WorldPacket data(SMSG_ZONE_UNDER_ATTACK, 4);
    data << uint32(GetZoneId());
    sWorld.SendGlobalMessage(&data, NULL, (enemy_team == ALLIANCE ? HORDE : ALLIANCE));
}

void Creature::SetInCombatWithZone()
{
    if (!CanHaveThreatList())
    {
        sLog.outError("Creature entry %u call SetInCombatWithZone but creature cannot have threat list.", GetEntry());
        return;
    }

    Map* pMap = GetMap();

    if (!pMap->IsDungeon())
    {
        sLog.outError("Creature entry %u call SetInCombatWithZone for map (id: %u) that isn't an instance.", GetEntry(), pMap->GetId());
        return;
    }

    Map::PlayerList const &PlList = pMap->GetPlayers();

    if (PlList.isEmpty())
        return;

    for(Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
    {
        if (Player* pPlayer = i->getSource())
        {
            if (pPlayer->isGameMaster())
                continue;

            if (pPlayer->isAlive() && !IsFriendlyTo(pPlayer))
            {
                pPlayer->SetInCombatWith(this);
                AddThreat(pPlayer);
            }
        }
    }
}

Unit* Creature::SelectAttackingTarget(AttackingTarget target, uint32 position) const
{
    if (!CanHaveThreatList())
        return NULL;

    //ThreatList m_threatlist;
    ThreatList const& threatlist = getThreatManager().getThreatList();
    ThreatList::const_iterator i = threatlist.begin();
    ThreatList::const_reverse_iterator r = threatlist.rbegin();

    if (position >= threatlist.size() || !threatlist.size())
        return NULL;

    switch(target)
    {
        case ATTACKING_TARGET_RANDOM:
        {
            advance(i, position + (rand() % (threatlist.size() - position)));
            return GetMap()->GetUnit((*i)->getUnitGuid());
        }
        case ATTACKING_TARGET_TOPAGGRO:
        {
            advance(i, position);
            return GetMap()->GetUnit((*i)->getUnitGuid());
        }
        case ATTACKING_TARGET_BOTTOMAGGRO:
        {
            advance(r, position);
            return GetMap()->GetUnit((*r)->getUnitGuid());
        }
        // TODO: implement these
        //case ATTACKING_TARGET_RANDOM_PLAYER:
        //case ATTACKING_TARGET_TOPAGGRO_PLAYER:
        //case ATTACKING_TARGET_BOTTOMAGGRO_PLAYER:
    }

    return NULL;
}

void Creature::_AddCreatureSpellCooldown(uint32 spell_id, time_t end_time)
{
    m_CreatureSpellCooldowns[spell_id] = end_time;
}

void Creature::_AddCreatureCategoryCooldown(uint32 category, time_t apply_time)
{
    m_CreatureCategoryCooldowns[category] = apply_time;
}

void Creature::AddCreatureSpellCooldown(uint32 spellid)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellid);
    if(!spellInfo)
        return;

    uint32 cooldown = GetSpellRecoveryTime(spellInfo);
    if(cooldown)
        _AddCreatureSpellCooldown(spellid, time(NULL) + cooldown/IN_MILLISECONDS);

    if(spellInfo->Category)
        _AddCreatureCategoryCooldown(spellInfo->Category, time(NULL));
}

bool Creature::HasCategoryCooldown(uint32 spell_id) const
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);
    if(!spellInfo)
        return false;

    CreatureSpellCooldowns::const_iterator itr = m_CreatureCategoryCooldowns.find(spellInfo->Category);
    return (itr != m_CreatureCategoryCooldowns.end() && time_t(itr->second + (spellInfo->CategoryRecoveryTime / IN_MILLISECONDS)) > time(NULL));
}

bool Creature::HasSpellCooldown(uint32 spell_id) const
{
    CreatureSpellCooldowns::const_iterator itr = m_CreatureSpellCooldowns.find(spell_id);
    return (itr != m_CreatureSpellCooldowns.end() && itr->second > time(NULL)) || HasCategoryCooldown(spell_id);
}

bool Creature::IsInEvadeMode() const
{
    return !i_motionMaster.empty() && i_motionMaster.GetCurrentMovementGeneratorType() == HOME_MOTION_TYPE;
}

bool Creature::HasSpell(uint32 spellID) const
{
    uint8 i;
    for(i = 0; i < CREATURE_MAX_SPELLS; ++i)
        if(spellID == m_spells[i])
            break;
    return i < CREATURE_MAX_SPELLS;                         // break before end of iteration of known spells
}

time_t Creature::GetRespawnTimeEx() const
{
    time_t now = time(NULL);
    if(m_respawnTime > now)                                 // dead (no corpse)
        return m_respawnTime;
    else if (m_corpseDecayTimer > 0)                        // dead (corpse)
        return now + m_respawnDelay + m_corpseDecayTimer / IN_MILLISECONDS;
    else
        return now;
}

void Creature::GetRespawnCoord( float &x, float &y, float &z, float* ori, float* dist ) const
{
    if (m_DBTableGuid)
    {
        if (CreatureData const* data = sObjectMgr.GetCreatureData(GetDBTableGUIDLow()))
        {
            x = data->posX;
            y = data->posY;
            z = data->posZ;
            if (ori)
                *ori = data->orientation;
            if (dist)
                *dist = GetRespawnRadius();

            return;
        }
    }

    float orient;

    GetSummonPoint(x, y, z, orient);

    if (ori)
        *ori = orient;
    if (dist)
        *dist = GetRespawnRadius();
}

void Creature::AllLootRemovedFromCorpse()
{
    if (lootForBody && !HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE))
    {
        uint32 corpseLootedDelay;

        if (!lootForSkin)                                   // corpse was not skinned -> apply corpseLootedDelay
        {
            // use a static spawntimesecs/3 modifier (guessed/made up value) unless config are more than 0.0
            // spawntimesecs=3min:  corpse decay after 1min
            // spawntimesecs=4hour: corpse decay after 1hour 20min
            if (sWorld.getConfig(CONFIG_FLOAT_RATE_CORPSE_DECAY_LOOTED) > 0.0f)
                corpseLootedDelay = (uint32)((m_corpseDelay * IN_MILLISECONDS) * sWorld.getConfig(CONFIG_FLOAT_RATE_CORPSE_DECAY_LOOTED));
            else
                corpseLootedDelay = (m_respawnDelay*IN_MILLISECONDS) /3;
        }
        else                                                // corpse was skinned, corpse will despawn next update
            corpseLootedDelay = 0;

        // if m_respawnTime is not expired already
        if (m_respawnTime >= time(NULL))
        {
            // if spawntimesecs is larger than default corpse delay always use corpseLootedDelay
            if (m_respawnDelay > m_corpseDelay)
            {
                m_corpseDecayTimer = corpseLootedDelay;
            }
            else
            {
                // if m_respawnDelay is relatively short and corpseDecayTimer is larger than corpseLootedDelay
                if (m_corpseDecayTimer > corpseLootedDelay)
                    m_corpseDecayTimer = corpseLootedDelay;
            }
        }
        else
        {
            m_corpseDecayTimer = 0;

            // TODO: reaching here, means mob will respawn at next tick.
            // This might be a place to set some aggro delay so creature has
            // ~5 seconds before it can react to hostile surroundings.

            // It's worth noting that it will not be fully correct either way.
            // At this point another "instance" of the creature are presumably expected to
            // be spawned already, while this corpse will not appear in respawned form.
        }
    }
}

uint32 Creature::GetLevelForTarget( Unit const* target ) const
{
    if(!IsWorldBoss())
        return Unit::GetLevelForTarget(target);

    uint32 level = target->getLevel()+sWorld.getConfig(CONFIG_UINT32_WORLD_BOSS_LEVEL_DIFF);
    if(level < 1)
        return 1;
    if(level > 255)
        return 255;
    return level;
}

std::string Creature::GetAIName() const
{
    return ObjectMgr::GetCreatureTemplate(GetEntry())->AIName;
}

std::string Creature::GetScriptName() const
{
    return sScriptMgr.GetScriptName(GetScriptId());
}

uint32 Creature::GetScriptId() const
{
    return ObjectMgr::GetCreatureTemplate(GetEntry())->ScriptID;
}

VendorItemData const* Creature::GetVendorItems() const
{
    return sObjectMgr.GetNpcVendorItemList(GetEntry());
}

VendorItemData const* Creature::GetVendorTemplateItems() const
{
    uint32 vendorId = GetCreatureInfo()->vendorId;
    return vendorId ? sObjectMgr.GetNpcVendorTemplateItemList(vendorId) : NULL;
}

uint32 Creature::GetVendorItemCurrentCount(VendorItem const* vItem)
{
    if(!vItem->maxcount)
        return vItem->maxcount;

    VendorItemCounts::iterator itr = m_vendorItemCounts.begin();
    for(; itr != m_vendorItemCounts.end(); ++itr)
        if(itr->itemId==vItem->item)
            break;

    if(itr == m_vendorItemCounts.end())
        return vItem->maxcount;

    VendorItemCount* vCount = &*itr;

    time_t ptime = time(NULL);

    if( vCount->lastIncrementTime + vItem->incrtime <= ptime )
    {
        ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(vItem->item);

        uint32 diff = uint32((ptime - vCount->lastIncrementTime)/vItem->incrtime);
        if((vCount->count + diff * pProto->BuyCount) >= vItem->maxcount )
        {
            m_vendorItemCounts.erase(itr);
            return vItem->maxcount;
        }

        vCount->count += diff * pProto->BuyCount;
        vCount->lastIncrementTime = ptime;
    }

    return vCount->count;
}

uint32 Creature::UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count)
{
    if(!vItem->maxcount)
        return 0;

    VendorItemCounts::iterator itr = m_vendorItemCounts.begin();
    for(; itr != m_vendorItemCounts.end(); ++itr)
        if(itr->itemId==vItem->item)
            break;

    if(itr == m_vendorItemCounts.end())
    {
        uint32 new_count = vItem->maxcount > used_count ? vItem->maxcount-used_count : 0;
        m_vendorItemCounts.push_back(VendorItemCount(vItem->item,new_count));
        return new_count;
    }

    VendorItemCount* vCount = &*itr;

    time_t ptime = time(NULL);

    if( vCount->lastIncrementTime + vItem->incrtime <= ptime )
    {
        ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(vItem->item);

        uint32 diff = uint32((ptime - vCount->lastIncrementTime)/vItem->incrtime);
        if((vCount->count + diff * pProto->BuyCount) < vItem->maxcount )
            vCount->count += diff * pProto->BuyCount;
        else
            vCount->count = vItem->maxcount;
    }

    vCount->count = vCount->count > used_count ? vCount->count-used_count : 0;
    vCount->lastIncrementTime = ptime;
    return vCount->count;
}

TrainerSpellData const* Creature::GetTrainerTemplateSpells() const
{
    uint32 trainerId = GetCreatureInfo()->trainerId;
    return trainerId ? sObjectMgr.GetNpcTrainerTemplateSpells(trainerId) : NULL;
}

TrainerSpellData const* Creature::GetTrainerSpells() const
{
    return sObjectMgr.GetNpcTrainerSpells(GetEntry());
}

// overwrite WorldObject function for proper name localization
const char* Creature::GetNameForLocaleIdx(int32 loc_idx) const
{
    if (loc_idx >= 0)
    {
        CreatureLocale const *cl = sObjectMgr.GetCreatureLocale(GetEntry());
        if (cl)
        {
            if (cl->Name.size() > (size_t)loc_idx && !cl->Name[loc_idx].empty())
                return cl->Name[loc_idx].c_str();
        }
    }

    return GetName();
}

void Creature::SetActiveObjectState( bool on )
{
    if(m_isActiveObject==on)
        return;

    bool world = IsInWorld();

    Map* map;
    if(world)
    {
        map = GetMap();
        map->Remove(this,false);
    }

    m_isActiveObject = on;

    if(world)
        map->Add(this);
}

void Creature::SendMonsterMoveWithSpeedToCurrentDestination(Player* player)
{
    float x, y, z;
    if(GetMotionMaster()->GetDestination(x, y, z))
        SendMonsterMoveWithSpeed(x, y, z, 0, player);
}

void Creature::SendAreaSpiritHealerQueryOpcode(Player *pl)
{
    uint32 next_resurrect = 0;
    if (Spell* pcurSpell = GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        next_resurrect = pcurSpell->GetCastedTime();
    WorldPacket data(SMSG_AREA_SPIRIT_HEALER_TIME, 8 + 4);
    data << GetGUID() << next_resurrect;
    pl->SendDirectMessage(&data);
}

void Creature::RelocationNotify()
{
    MaNGOS::CreatureRelocationNotifier relocationNotifier(*this);
    float radius = MAX_CREATURE_ATTACK_RADIUS * sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO);
    Cell::VisitAllObjects(this, relocationNotifier, radius);
}

void Creature::ApplyGameEventSpells(GameEventCreatureData const* eventData, bool activated)
{
    uint32 cast_spell = activated ? eventData->spell_id_start : eventData->spell_id_end;
    uint32 remove_spell = activated ? eventData->spell_id_end : eventData->spell_id_start;

    if (remove_spell)
        if (SpellEntry const* spellEntry = sSpellStore.LookupEntry(remove_spell))
            if (IsSpellAppliesAura(spellEntry))
                RemoveAurasDueToSpell(remove_spell);

    if (cast_spell)
        CastSpell(this, cast_spell, true);
}
