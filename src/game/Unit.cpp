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

#include "Unit.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "Player.h"
#include "Creature.h"
#include "Spell.h"
#include "Group.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "Formulas.h"
#include "Pet.h"
#include "Util.h"
#include "Totem.h"
#include "BattleGround.h"
#include "InstanceData.h"
#include "InstanceSaveMgr.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Path.h"
#include "Traveller.h"
#include "Vehicle.h"
#include "VMapFactory.h"
#include "MovementGenerator.h"
#include "Transports.h"

#include <math.h>
#include <stdarg.h>

float baseMoveSpeed[MAX_MOVE_TYPE] =
{
    2.5f,                                                   // MOVE_WALK
    7.0f,                                                   // MOVE_RUN
    2.5f,                                                   // MOVE_RUN_BACK
    4.722222f,                                              // MOVE_SWIM
    4.5f,                                                   // MOVE_SWIM_BACK
    3.141594f,                                              // MOVE_TURN_RATE
    7.0f,                                                   // MOVE_FLIGHT
    4.5f,                                                   // MOVE_FLIGHT_BACK
    3.14f                                                   // MOVE_PITCH_RATE
};

////////////////////////////////////////////////////////////
// Methods of class MovementInfo

void MovementInfo::Read(ByteBuffer &data)
{
    data >> moveFlags;
    data >> moveFlags2;
    data >> time;
    data >> pos.x;
    data >> pos.y;
    data >> pos.z;
    data >> pos.o;

    if(HasMovementFlag(MOVEFLAG_ONTRANSPORT))
    {
        data >> t_guid.ReadAsPacked();
        data >> t_pos.x;
        data >> t_pos.y;
        data >> t_pos.z;
        data >> t_pos.o;
        data >> t_time;
        data >> t_seat;

        if(moveFlags2 & MOVEFLAG2_INTERP_MOVEMENT)
            data >> t_time2;
    }

    if((HasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING))) || (moveFlags2 & MOVEFLAG2_ALLOW_PITCHING))
    {
        data >> s_pitch;
    }

    data >> fallTime;

    if(HasMovementFlag(MOVEFLAG_FALLING))
    {
        data >> jump.velocity;
        data >> jump.sinAngle;
        data >> jump.cosAngle;
        data >> jump.xyspeed;
    }

    if(HasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
    {
        data >> u_unk1;
    }
}

void MovementInfo::Write(ByteBuffer &data) const
{
    data << moveFlags;
    data << moveFlags2;
    data << time;
    data << pos.x;
    data << pos.y;
    data << pos.z;
    data << pos.o;

    if(HasMovementFlag(MOVEFLAG_ONTRANSPORT))
    {
        data << t_guid.WriteAsPacked();
        data << t_pos.x;
        data << t_pos.y;
        data << t_pos.z;
        data << t_pos.o;
        data << t_time;
        data << t_seat;

        if(moveFlags2 & MOVEFLAG2_INTERP_MOVEMENT)
            data << t_time2;
    }

    if((HasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING))) || (moveFlags2 & MOVEFLAG2_ALLOW_PITCHING))
    {
        data << s_pitch;
    }

    data << fallTime;

    if(HasMovementFlag(MOVEFLAG_FALLING))
    {
        data << jump.velocity;
        data << jump.sinAngle;
        data << jump.cosAngle;
        data << jump.xyspeed;
    }

    if(HasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
    {
        data << u_unk1;
    }
}

////////////////////////////////////////////////////////////
// Methods of class GlobalCooldownMgr

bool GlobalCooldownMgr::HasGlobalCooldown(SpellEntry const* spellInfo) const
{
    GlobalCooldownList::const_iterator itr = m_GlobalCooldowns.find(spellInfo->StartRecoveryCategory);
    return itr != m_GlobalCooldowns.end() && itr->second.duration && WorldTimer::getMSTimeDiff(itr->second.cast_time, WorldTimer::getMSTime()) < itr->second.duration;
}

void GlobalCooldownMgr::AddGlobalCooldown(SpellEntry const* spellInfo, uint32 gcd)
{
    m_GlobalCooldowns[spellInfo->StartRecoveryCategory] = GlobalCooldown(gcd, WorldTimer::getMSTime());
}

void GlobalCooldownMgr::CancelGlobalCooldown(SpellEntry const* spellInfo)
{
    m_GlobalCooldowns[spellInfo->StartRecoveryCategory].duration = 0;
}

////////////////////////////////////////////////////////////
// Methods of class Unit

Unit::Unit()
: WorldObject(), i_motionMaster(this), m_ThreatManager(this), m_HostileRefManager(this)
{
    m_objectType |= TYPEMASK_UNIT;
    m_objectTypeId = TYPEID_UNIT;

    m_updateFlag = (UPDATEFLAG_HIGHGUID | UPDATEFLAG_LIVING | UPDATEFLAG_HAS_POSITION);

    m_attackTimer[BASE_ATTACK]   = 0;
    m_attackTimer[OFF_ATTACK]    = 0;
    m_attackTimer[RANGED_ATTACK] = 0;
    m_modAttackSpeedPct[BASE_ATTACK] = 1.0f;
    m_modAttackSpeedPct[OFF_ATTACK] = 1.0f;
    m_modAttackSpeedPct[RANGED_ATTACK] = 1.0f;

    m_extraAttacks = 0;

    m_state = 0;
    m_deathState = ALIVE;

    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
        m_currentSpells[i] = NULL;

    m_castCounter = 0;

    m_addDmgOnce = 0;

    for(int i = 0; i < MAX_TOTEM_SLOT; ++i)
        m_TotemSlot[i] = 0;

    m_ObjectSlot[0] = m_ObjectSlot[1] = m_ObjectSlot[2] = m_ObjectSlot[3] = 0;
    //m_Aura = NULL;
    //m_AurasCheck = 2000;
    //m_removeAuraTimer = 4;
    m_spellAuraHoldersUpdateIterator = m_spellAuraHolders.end();
    m_AuraFlags = 0;

    m_Visibility = VISIBILITY_ON;

    m_detectInvisibilityMask = 0;
    m_invisibilityMask = 0;
    m_transform = 0;
    m_canModifyStats = false;

    for (int i = 0; i < MAX_SPELL_IMMUNITY; ++i)
        m_spellImmune[i].clear();
    for (int i = 0; i < UNIT_MOD_END; ++i)
    {
        m_auraModifiersGroup[i][BASE_VALUE] = 0.0f;
        m_auraModifiersGroup[i][BASE_PCT] = 1.0f;
        m_auraModifiersGroup[i][TOTAL_VALUE] = 0.0f;
        m_auraModifiersGroup[i][TOTAL_PCT] = 1.0f;
    }
                                                            // implement 50% base damage from offhand
    m_auraModifiersGroup[UNIT_MOD_DAMAGE_OFFHAND][TOTAL_PCT] = 0.5f;

    for (int i = 0; i < MAX_ATTACK; ++i)
    {
        m_weaponDamage[i][MINDAMAGE] = BASE_MINDAMAGE;
        m_weaponDamage[i][MAXDAMAGE] = BASE_MAXDAMAGE;
    }
    for (int i = 0; i < MAX_STATS; ++i)
        m_createStats[i] = 0.0f;

    m_attacking = NULL;
    m_modMeleeHitChance = 0.0f;
    m_modRangedHitChance = 0.0f;
    m_modSpellHitChance = 0.0f;
    m_baseSpellCritChance = 5;

    m_CombatTimer = 0;
    m_lastManaUseTimer = 0;

    //m_victimThreat = 0.0f;
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        m_threatModifier[i] = 1.0f;
    m_isSorted = true;
    for (int i = 0; i < MAX_MOVE_TYPE; ++i)
        m_speed_rate[i] = 1.0f;

    m_charmInfo = NULL;

    m_ThreatRedirectionPercent = 0;
    m_misdirectionTargetGUID = 0;

    // remove aurastates allowing special moves
    for(int i=0; i < MAX_REACTIVE; ++i)
        m_reactiveTimer[i] = 0;

    m_transport = NULL;

    m_pVehicle = NULL;
    m_pVehicleKit = NULL;

    m_comboPoints = 0;

    // Frozen Mod
    m_spoofSamePlayerFaction = false;
    // Frozen Mod

}

Unit::~Unit()
{
    // set current spells as deletable
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
    {
        if (m_currentSpells[i])
        {
            m_currentSpells[i]->SetReferencedFromCurrent(false);
            m_currentSpells[i] = NULL;
        }
    }

    if (m_charmInfo)
        delete m_charmInfo;

    // those should be already removed at "RemoveFromWorld()" call
    MANGOS_ASSERT(m_gameObj.size() == 0);
    MANGOS_ASSERT(m_dynObjGUIDs.size() == 0);
    MANGOS_ASSERT(m_deletedAuras.size() == 0);
    MANGOS_ASSERT(m_deletedHolders.size() == 0);
}

void Unit::Update( uint32 update_diff, uint32 p_time )
{
    if(!IsInWorld())
        return;
    
    /*if(p_time > m_AurasCheck)
    {
    m_AurasCheck = 2000;
    _UpdateAura();
    }else
    m_AurasCheck -= p_time;*/

    // WARNING! Order of execution here is important, do not change.
    // Spells must be processed with event system BEFORE they go to _UpdateSpells.
    // Or else we may have some SPELL_STATE_FINISHED spells stalled in pointers, that is bad.
    m_Events.Update( update_diff );
    _UpdateSpells( update_diff );

    CleanupDeletedAuras();

    if (m_lastManaUseTimer)
    {
        if (update_diff >= m_lastManaUseTimer)
            m_lastManaUseTimer = 0;
        else
            m_lastManaUseTimer -= update_diff;
    }

    if (CanHaveThreatList())
        getThreatManager().UpdateForClient(update_diff);

    // update combat timer only for players and pets
    if (isInCombat() && (GetTypeId() == TYPEID_PLAYER || ((Creature*)this)->IsPet() || ((Creature*)this)->isCharmed()))
    {
        // Check UNIT_STAT_MELEE_ATTACKING or UNIT_STAT_CHASE (without UNIT_STAT_FOLLOW in this case) so pets can reach far away
        // targets without stopping half way there and running off.
        // These flags are reset after target dies or another command is given.
        if (m_HostileRefManager.isEmpty())
        {
            // m_CombatTimer set at aura start and it will be freeze until aura removing
            if (m_CombatTimer <= update_diff)
                CombatStop();
            else
                m_CombatTimer -= update_diff;
        }
    }

    if (uint32 base_att = getAttackTimer(BASE_ATTACK))
    {
        setAttackTimer(BASE_ATTACK, (update_diff >= base_att ? 0 : base_att - update_diff) );
    }

    // update abilities available only for fraction of time
    UpdateReactives( update_diff );

    ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, GetHealth() < GetMaxHealth()*0.20f);
    ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, GetHealth() < GetMaxHealth()*0.35f);
    ModifyAuraState(AURA_STATE_HEALTH_ABOVE_75_PERCENT, GetHealth() > GetMaxHealth()*0.75f);

    i_motionMaster.UpdateMotion(p_time);
}

bool Unit::haveOffhandWeapon() const
{
    if (!CanUseEquippedWeapon(OFF_ATTACK))
        return false;

    if(GetTypeId() == TYPEID_PLAYER)
        return ((Player*)this)->GetWeaponForAttack(OFF_ATTACK,true,true);
    else
        return false;
}

void Unit::SendMonsterMove(float NewPosX, float NewPosY, float NewPosZ, SplineType type, SplineFlags flags, uint32 Time, Player* player, ...)
{

    va_list vargs;
    va_start(vargs,player);

    float moveTime = (float)Time;

    WorldPacket data( SMSG_MONSTER_MOVE, (41 + GetPackGUID().size()) );
    data << GetPackGUID();
    data << uint8(0);                                       // new in 3.1 bool, used to toggle MOVEFLAG2_UNK4 = 0x0040 on client side
    data << GetPositionX() << GetPositionY() << GetPositionZ();
    data << uint32(WorldTimer::getMSTime());

    data << uint8(type);                                    // unknown
    switch(type)
    {
        case SPLINETYPE_NORMAL:                             // normal packet
            break;
        case SPLINETYPE_STOP:                               // stop packet (raw pos?)
            va_end(vargs);
            SendMessageToSet( &data, true );
            return;
        case SPLINETYPE_FACINGSPOT:                         // facing spot, not used currently
        {
            data << float(va_arg(vargs,double));
            data << float(va_arg(vargs,double));
            data << float(va_arg(vargs,double));
            break;
        }
        case SPLINETYPE_FACINGTARGET:
            data << uint64(va_arg(vargs,uint64));
            break;
        case SPLINETYPE_FACINGANGLE:                        // not used currently
            data << float(va_arg(vargs,double));            // facing angle
            break;
    }

    data << uint32(flags);                                  // splineflags
    data << uint32(moveTime);                               // Time in between points
    if (flags & SPLINEFLAG_TRAJECTORY)
    {
        data << float(va_arg(vargs, double));               // Z jump speed
        data << uint32(0);                                  // walk time after jump
    }
    data << uint32(1);                                      // 1 single waypoint
    data << NewPosX << NewPosY << NewPosZ;                  // the single waypoint Point B

    va_end(vargs);

    if(player)
        player->GetSession()->SendPacket(&data);
    else
        SendMessageToSet( &data, true );
}

void Unit::SendMonsterMoveJump(float NewPosX, float NewPosY, float NewPosZ, float vert_speed, uint32 flags, uint32 Time, Player* player)
{
    float moveTime = Time;
    flags |= SPLINEFLAG_TRAJECTORY;

    WorldPacket data( SMSG_MONSTER_MOVE, (49 + GetPackGUID().size()) );
    data << GetPackGUID();
    data << uint8(0);                                       // new in 3.1
    data << GetPositionX() << GetPositionY() << GetPositionZ();
    data << uint32(WorldTimer::getMSTime());

    data << uint8(0);                                    // unknown

    data << uint32(flags);

    if(flags & SPLINEFLAG_WALKMODE)
        moveTime *= 1.05f;

    data << uint32(moveTime);                               // Time in between points

    if(flags & SPLINEFLAG_TRAJECTORY)
    {
        data << float(vert_speed);
        data << uint32(0);
    }

    data << uint32(1);                                      // 1 single waypoint
    data << NewPosX << NewPosY << NewPosZ;                  // the single waypoint Point B

    if(player)
        player->GetSession()->SendPacket(&data);
    else
        SendMessageToSet( &data, true );
}

void Unit::SendMonsterMoveWithSpeed(float x, float y, float z, uint32 transitTime, Player* player)
{
    if (!transitTime)
    {
        if(GetTypeId()==TYPEID_PLAYER)
        {
            Traveller<Player> traveller(*(Player*)this);
            transitTime = traveller.GetTotalTravelTimeTo(x, y, z);
        }
        else
        {
            Traveller<Creature> traveller(*(Creature*)this);
            transitTime = traveller.GetTotalTravelTimeTo(x, y, z);
        }
    }
    //float orientation = (float)atan2((double)dy, (double)dx);
    SplineFlags flags = GetTypeId() == TYPEID_PLAYER ? SPLINEFLAG_WALKMODE : ((Creature*)this)->GetSplineFlags();
    SendMonsterMove(x, y, z, SPLINETYPE_NORMAL, flags, transitTime, player);
}

bool Unit::SetPosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if (!MaNGOS::IsValidMapCoord(x, y, z, orientation))
    {
        DEBUG_LOG("Unit::SetPosition(%f, %f, %f, %f, %d) .. bad coordinates for unit %d!", x, y, z, orientation, teleport, GetGUIDLow());
        return false;
    }

    bool turn = GetOrientation() != orientation;
    bool relocate = (teleport || GetPositionX() != x || GetPositionY() != y || GetPositionZ() != z);

    if (turn)
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);

    if (relocate)
    {
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE);

        if (GetTypeId() == TYPEID_PLAYER)
            GetMap()->PlayerRelocation((Player*)this, x, y, z, orientation);
        else
            GetMap()->CreatureRelocation((Creature*)this, x, y, z, orientation);
    }
    else if (turn)
        SetOrientation(orientation);

    if ((relocate || turn) && GetVehicleKit())
        GetVehicleKit()->RelocatePassengers(x, y, z, orientation);

    return relocate || turn;
}

void Unit::SendMonsterMoveTransport(WorldObject *transport, SplineType type, SplineFlags flags, uint32 moveTime, ...)
{
    va_list vargs;
    va_start(vargs, moveTime);

    WorldPacket data(SMSG_MONSTER_MOVE_TRANSPORT, 60);
    data << GetPackGUID();
    data << transport->GetPackGUID();
    data << uint8(m_movementInfo.GetTransportSeat());
    data << uint8(0);                                       // new in 3.1
    data << float(transport->GetPositionX());
    data << float(transport->GetPositionY());
    data << float(transport->GetPositionZ());
    data << uint32(WorldTimer::getMSTime());

    data << uint8(type);                                    // spline type

    switch(type)
    {
        case SPLINETYPE_NORMAL:                             // normal packet
            break;
        case SPLINETYPE_STOP:                               // stop packet (raw pos?)
            va_end(vargs);
            SendMessageToSet(&data, true);
            return;
        case SPLINETYPE_FACINGSPOT:                         // facing spot
            data << float(va_arg(vargs,double));
            data << float(va_arg(vargs,double));
            data << float(va_arg(vargs,double));
            break;
        case SPLINETYPE_FACINGTARGET:
            data << uint64(va_arg(vargs,uint64));
            break;
        case SPLINETYPE_FACINGANGLE:
            data << float(va_arg(vargs,double));            // facing angle
            break;
    }

    va_end(vargs);

    data << uint32(flags);

    data << uint32(moveTime);                               // Time in between points
    data << uint32(1);                                      // 1 single waypoint

    data << float(m_movementInfo.GetTransportPos()->x);
    data << float(m_movementInfo.GetTransportPos()->y);
    data << float(m_movementInfo.GetTransportPos()->z);

    SendMessageToSet(&data, true);
}

void Unit::SendHeartBeat(bool toSelf)
{
    WorldPacket data(MSG_MOVE_HEARTBEAT, 64);
    data << GetPackGUID();
    data << m_movementInfo;
    SendMessageToSet(&data, toSelf);
}

void Unit::resetAttackTimer(WeaponAttackType type)
{
    m_attackTimer[type] = uint32(GetAttackTime(type) * m_modAttackSpeedPct[type]);
}

bool Unit::canReachWithAttack(Unit *pVictim) const
{
    MANGOS_ASSERT(pVictim);
    float reach = GetFloatValue(UNIT_FIELD_COMBATREACH);
    if( reach <= 0.0f )
        reach = 1.0f;
    return IsWithinDistInMap(pVictim, reach);
}

void Unit::RemoveSpellsCausingAura(AuraType auraType)
{
    for (AuraList::const_iterator iter = m_modAuras[auraType].begin(); iter != m_modAuras[auraType].end();)
    {
        RemoveAurasDueToSpell((*iter)->GetId());
        iter = m_modAuras[auraType].begin();
    }
}

void Unit::RemoveSpellsCausingAura(AuraType auraType, SpellAuraHolder* except)
{
    for (AuraList::const_iterator iter = m_modAuras[auraType].begin(); iter != m_modAuras[auraType].end();)
    {
        // skip `except` aura
        if ((*iter)->GetHolder() == except)
        {
            ++iter;
            continue;
        }

        RemoveAurasDueToSpell((*iter)->GetId(), except);
        iter = m_modAuras[auraType].begin();
    }
}

bool Unit::HasAuraType(AuraType auraType) const
{
    return (!m_modAuras[auraType].empty());
}

/* Called by DealDamage for auras that have a chance to be dispelled on damage taken. */
void Unit::RemoveSpellbyDamageTaken(AuraType auraType, uint32 damage)
{
    if(!HasAuraType(auraType))
        return;

    // The chance to dispel an aura depends on the damage taken with respect to the casters level.
    uint32 max_dmg = getLevel() > 8 ? 25 * getLevel() - 150 : 50;

    AuraList const& typeAuras = GetAurasByType(auraType);
    for (AuraList::const_iterator iter = typeAuras.begin(); iter != typeAuras.end(); ++iter)
    {
        Unit *caster = (*iter)->GetCaster();

        if (!caster)
            continue;

        AuraList const& mOverrideClassScript = caster->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for(AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue == 7801 && (*i)->isAffectedOnSpell((*iter)->GetSpellProto()))
                max_dmg += (*i)->GetModifier()->m_amount * max_dmg / 100;
        }
    }

    float chance = float(damage) / max_dmg * 100.0f;
    if (roll_chance_f(chance))
        RemoveSpellsCausingAura(auraType);
}

void Unit::DealDamageMods(Unit *pVictim, uint32 &damage, uint32* absorb)
{
    if (!pVictim->isAlive() || pVictim->IsTaxiFlying() || (pVictim->GetTypeId() == TYPEID_UNIT && ((Creature*)pVictim)->IsInEvadeMode()))
    {
        if (absorb)
            *absorb += damage;
        damage = 0;
        return;
    }

    //You don't lose health from damage taken from another player while in a sanctuary
    //You still see it in the combat log though
    if (!IsAllowedDamageInArea(pVictim))
    {
        if(absorb)
            *absorb += damage;
        damage = 0;
    }

    uint32 originalDamage = damage;

    //Script Event damage Deal
    if( GetTypeId()== TYPEID_UNIT && ((Creature *)this)->AI())
        ((Creature *)this)->AI()->DamageDeal(pVictim, damage);
    //Script Event damage taken
    if( pVictim->GetTypeId()== TYPEID_UNIT && ((Creature *)pVictim)->AI() )
        ((Creature *)pVictim)->AI()->DamageTaken(this, damage);

    if(absorb && originalDamage > damage)
        *absorb += (originalDamage - damage);
}

uint32 Unit::DealDamage(Unit *pVictim, uint32 damage, CleanDamage const* cleanDamage, DamageEffectType damagetype, SpellSchoolMask damageSchoolMask, SpellEntry const *spellProto, bool durabilityLoss)
{
    // remove affects from victim (including from 0 damage and DoTs)
    if(pVictim != this)
        pVictim->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);

    // Divine Storm heal hack
    if ( spellProto && spellProto->Id == 53385 )
    {
        int32 divineDmg = damage * (25 + (HasAura(63220) ? 15 : 0)) / 100; //25%, if has Glyph of Divine Storm -> 40%
        CastCustomSpell(this, 54171, &divineDmg, NULL, NULL, true);
    }

    // remove affects from attacker at any non-DoT damage (including 0 damage)
    if( damagetype != DOT)
    {
        RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
        RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

        if(pVictim != this)
            RemoveSpellsCausingAura(SPELL_AURA_MOD_INVISIBILITY);

        if(pVictim->GetTypeId() == TYPEID_PLAYER && !pVictim->IsStandState() && !pVictim->hasUnitState(UNIT_STAT_STUNNED))
            pVictim->SetStandState(UNIT_STAND_STATE_STAND);
    }

    // Blessed Life talent of Paladin
    if( pVictim->GetTypeId() == TYPEID_PLAYER )
    {
        Unit::AuraList const& BlessedLife = pVictim->GetAurasByType(SPELL_AURA_PROC_TRIGGER_SPELL);
        for(Unit::AuraList::const_iterator i = BlessedLife.begin(); i != BlessedLife.end(); ++i)
            if((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_PALADIN && (*i)->GetSpellProto()->SpellIconID == 2137)
                if( urand(0,100) < (*i)->GetSpellProto()->procChance )
                    damage *= 0.5;
    }

    if(!damage)
    {
        if (!cleanDamage)
            return 0;

        if (!cleanDamage->absorb)
        {
            // Rage from physical damage received .
            if(cleanDamage->damage && (damageSchoolMask & SPELL_SCHOOL_MASK_NORMAL) && pVictim->GetTypeId() == TYPEID_PLAYER && (pVictim->getPowerType() == POWER_RAGE))
                ((Player*)pVictim)->RewardRage(cleanDamage->damage, 0, false);

            return 0;
        }
    }
    if (!spellProto || !IsSpellHaveAura(spellProto,SPELL_AURA_MOD_FEAR))
        pVictim->RemoveSpellbyDamageTaken(SPELL_AURA_MOD_FEAR, damage);
    // root type spells do not dispel the root effect
    if (!spellProto || !(spellProto->Mechanic == MECHANIC_ROOT || IsSpellHaveAura(spellProto,SPELL_AURA_MOD_ROOT)))
        pVictim->RemoveSpellbyDamageTaken(SPELL_AURA_MOD_ROOT, damage);

    // no xp,health if type 8 /critters/
    if (pVictim->GetTypeId() == TYPEID_UNIT && pVictim->GetCreatureType() == CREATURE_TYPE_CRITTER)
    {
        // TODO: fix this part
        // Critter may not die of damage taken, instead expect it to run away (no fighting back)
        // If (this) is TYPEID_PLAYER, (this) will enter combat w/victim, but after some time, automatically leave combat.
        // It is unclear how it should work for other cases.

        ((Creature*)pVictim)->SetLootRecipient(this);

        pVictim->SetDeathState(JUST_DIED);
        pVictim->SetHealth(0);

        // allow loot only if has loot_id in creature_template
        ((Creature*)pVictim)->PrepareBodyLootState();
        ((Creature*)pVictim)->AllLootRemovedFromCorpse();

        // some critters required for quests (need normal entry instead possible heroic in any cases)
        if (GetTypeId() == TYPEID_PLAYER)
        {
            if (CreatureInfo const* normalInfo = ObjectMgr::GetCreatureTemplate(pVictim->GetEntry()))
                ((Player*)this)->KilledMonster(normalInfo, pVictim->GetObjectGuid());
        }

        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE, "DealDamage critter, critter dies");

        return damage;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"DealDamageStart");

    // share damage by auras
    AuraList const& vShareDamageAuras = pVictim->GetAurasByType(SPELL_AURA_SHARE_DAMAGE_PCT);
    for (AuraList::const_iterator itr = vShareDamageAuras.begin(); itr != vShareDamageAuras.end(); ++itr)
    {
        if (Unit* shareTarget = (*itr)->GetCaster())
        {
            if (shareTarget != pVictim && ((*itr)->GetMiscValue() & damageSchoolMask))
            {
                SpellEntry const * shareSpell = (*itr)->GetSpellProto();
                uint32 shareDamage = uint32(damage * (*itr)->GetModifier()->m_amount / 100.0f);
                DealDamageMods(shareTarget, shareDamage, NULL);
                DealDamage(shareTarget, shareDamage, 0, damagetype, GetSpellSchoolMask(shareSpell), spellProto, false);
            }
        }
    }

    uint32 health = pVictim->GetHealth();
    DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"deal dmg:%d to health:%d ",damage,health);

    // duel ends when player has 1 or less hp
    bool duel_hasEnded = false;
    if(pVictim->GetTypeId() == TYPEID_PLAYER && ((Player*)pVictim)->duel && damage >= (health-1))
    {
        // prevent kill only if killed in duel and killed by opponent or opponent controlled creature
        if(((Player*)pVictim)->duel->opponent==this || ((Player*)pVictim)->duel->opponent->GetObjectGuid() == GetOwnerGuid())
            damage = health-1;

        duel_hasEnded = true;
    }
    //Get in CombatState
    if(pVictim != this && damagetype != DOT)
    {
        SetInCombatWith(pVictim);
        pVictim->SetInCombatWith(this);

        if(Player* attackedPlayer = pVictim->GetCharmerOrOwnerPlayerOrPlayerItself())
            SetContestedPvP(attackedPlayer);
    }

    // Rage from Damage made (only from direct weapon damage)
    if( cleanDamage && damagetype==DIRECT_DAMAGE && this != pVictim && GetTypeId() == TYPEID_PLAYER && (getPowerType() == POWER_RAGE))
    {
        uint32 weaponSpeedHitFactor;

        switch(cleanDamage->attackType)
        {
            case BASE_ATTACK:
            {
                if(cleanDamage->hitOutCome == MELEE_HIT_CRIT)
                    weaponSpeedHitFactor = uint32(GetAttackTime(cleanDamage->attackType)/1000.0f * 7);
                else
                    weaponSpeedHitFactor = uint32(GetAttackTime(cleanDamage->attackType)/1000.0f * 3.5f);

                ((Player*)this)->RewardRage(damage + cleanDamage->absorb, weaponSpeedHitFactor, true);

                break;
            }
            case OFF_ATTACK:
            {
                if(cleanDamage->hitOutCome == MELEE_HIT_CRIT)
                    weaponSpeedHitFactor = uint32(GetAttackTime(cleanDamage->attackType)/1000.0f * 3.5f);
                else
                    weaponSpeedHitFactor = uint32(GetAttackTime(cleanDamage->attackType)/1000.0f * 1.75f);

                ((Player*)this)->RewardRage(damage + cleanDamage->absorb, weaponSpeedHitFactor, true);

                break;
            }
            case RANGED_ATTACK:
                break;
        }
    }

    if (GetTypeId() == TYPEID_PLAYER && this != pVictim)
    {
        Player *killer = ((Player*)this);

        /* process anticheat check */
        killer->GetAntiCheat()->DoAntiCheatCheck(CHECK_DAMAGE, 0, 0, damage);


        // in bg, count dmg if victim is also a player
        if (pVictim->GetTypeId()==TYPEID_PLAYER)
        {
            if (BattleGround *bg = killer->GetBattleGround())
            {
                // FIXME: kept by compatibility. don't know in BG if the restriction apply.
                bg->UpdatePlayerScore(killer, SCORE_DAMAGE_DONE, damage);
            }
        }

        killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE, damage, 0, pVictim);
        killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT, damage);
    }

    if (pVictim->GetTypeId() == TYPEID_PLAYER)
        ((Player*)pVictim)->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED, damage);

    if (pVictim->GetTypeId() == TYPEID_UNIT && !((Creature*)pVictim)->IsPet() && !((Creature*)pVictim)->HasLootRecipient())
        ((Creature*)pVictim)->SetLootRecipient(this);

    if (health <= damage)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"DealDamage: victim just died");

        // find player: owner of controlled `this` or `this` itself maybe
        // for loot will be sued only if group_tap==NULL
        Player *player_tap = GetCharmerOrOwnerPlayerOrPlayerItself();
        Group *group_tap = NULL;

        // find owner of pVictim, used for creature cases, AI calls
        Unit* pOwner = pVictim->GetCharmerOrOwner();

        // in creature kill case group/player tap stored for creature
        if (pVictim->GetTypeId() == TYPEID_UNIT)
        {
            group_tap = ((Creature*)pVictim)->GetGroupLootRecipient();

            if (Player* recipient = ((Creature*)pVictim)->GetOriginalLootRecipient())
                player_tap = recipient;
        }
        // in player kill case group tap selected by player_tap (killer-player itself, or charmer, or owner, etc)
        else
        {
            if (player_tap)
                group_tap = player_tap->GetGroup();
        }

        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)pVictim)->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED, health);
            if (player_tap)
                player_tap->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL,1,0,pVictim);
        }

        // call kill spell proc event (before real die and combat stop to triggering auras removed at death/combat stop)
        if(player_tap && player_tap != pVictim)
        {
            player_tap->ProcDamageAndSpell(pVictim, PROC_FLAG_KILL, PROC_FLAG_KILLED, PROC_EX_NONE, 0);

            WorldPacket data(SMSG_PARTYKILLLOG, (8+8));     //send event PARTY_KILL
            data << player_tap->GetObjectGuid();            //player with killing blow
            data << pVictim->GetObjectGuid();              //victim

            if (group_tap)
                group_tap->BroadcastPacket(&data, false, group_tap->GetMemberGroup(player_tap->GetObjectGuid()),player_tap->GetObjectGuid());

            player_tap->SendDirectMessage(&data);
        }

        // Reward player, his pets, and group/raid members
        if (player_tap != pVictim)
        {
            if (group_tap)
                group_tap->RewardGroupAtKill(pVictim, player_tap);
            else if (player_tap)
                player_tap->RewardSinglePlayerAtKill(pVictim);
        }

        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"DealDamageAttackStop");

        // stop combat
        pVictim->CombatStop();
        pVictim->getHostileRefManager().deleteReferences();

        bool damageFromSpiritOfRedemtionTalent = spellProto && spellProto->Id == 27795;

        // if talent known but not triggered (check priest class for speedup check)
        Aura* spiritOfRedemtionTalentReady = NULL;
        if( !damageFromSpiritOfRedemtionTalent &&           // not called from SPELL_AURA_SPIRIT_OF_REDEMPTION
            pVictim->GetTypeId()==TYPEID_PLAYER && pVictim->getClass()==CLASS_PRIEST )
        {
            AuraList const& vDummyAuras = pVictim->GetAurasByType(SPELL_AURA_DUMMY);
            for(AuraList::const_iterator itr = vDummyAuras.begin(); itr != vDummyAuras.end(); ++itr)
            {
                if((*itr)->GetSpellProto()->SpellIconID==1654)
                {
                    spiritOfRedemtionTalentReady = *itr;
                    break;
                }
            }
        }

        if (!spiritOfRedemtionTalentReady)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"SET JUST_DIED");
            pVictim->SetDeathState(JUST_DIED);
        }

        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"DealDamageHealth1");

        if(spiritOfRedemtionTalentReady)
        {
            // save value before aura remove
            uint32 ressSpellId = pVictim->GetUInt32Value(PLAYER_SELF_RES_SPELL);
            if(!ressSpellId)
                ressSpellId = ((Player*)pVictim)->GetResurrectionSpellId();

            //Remove all expected to remove at death auras (most important negative case like DoT or periodic triggers)
            pVictim->RemoveAllAurasOnDeath();

            // restore for use at real death
            pVictim->SetUInt32Value(PLAYER_SELF_RES_SPELL,ressSpellId);

            // FORM_SPIRITOFREDEMPTION and related auras
            pVictim->CastSpell(pVictim,27827,true,NULL,spiritOfRedemtionTalentReady);
        }
        else
            pVictim->SetHealth(0);

        // remember victim PvP death for corpse type and corpse reclaim delay
        // at original death (not at SpiritOfRedemtionTalent timeout)
        if( pVictim->GetTypeId()==TYPEID_PLAYER && !damageFromSpiritOfRedemtionTalent )
            ((Player*)pVictim)->SetPvPDeath(player_tap != NULL);

        // Call KilledUnit for creatures
        if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->AI())
            ((Creature*)this)->AI()->KilledUnit(pVictim);

        // Call AI OwnerKilledUnit (for any current summoned minipet/guardian/protector)
        PetOwnerKilledUnit(pVictim);

        // achievement stuff
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            if (GetTypeId() == TYPEID_UNIT)
                ((Player*)pVictim)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE, GetEntry());
            else if(GetTypeId() == TYPEID_PLAYER && pVictim != this)
                ((Player*)pVictim)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER, 1, ((Player*)this)->GetTeam());
        }

        // 10% durability loss on death
        // clean InHateListOf
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            // only if not player and not controlled by player pet. And not at BG
            if (durabilityLoss && !player_tap && !((Player*)pVictim)->InBattleGround())
            {
                DEBUG_LOG("We are dead, loosing 10 percents durability");
                ((Player*)pVictim)->DurabilityLossAll(0.10f,false);
                // durability lost message
                WorldPacket data(SMSG_DURABILITY_DAMAGE_DEATH, 0);
                ((Player*)pVictim)->GetSession()->SendPacket(&data);
            }
        }
        else                                                // creature died
        {
            DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"DealDamageNotPlayer");
            Creature *cVictim = (Creature*)pVictim;

            if(!cVictim->IsPet())
            {
                cVictim->DeleteThreatList();
                // only lootable if it has loot or can drop gold
                cVictim->PrepareBodyLootState();
                // may have no loot, so update death timer if allowed
                cVictim->AllLootRemovedFromCorpse();
            }

            // Call creature just died function
            if (cVictim->AI())
                cVictim->AI()->JustDied(this);

            if (cVictim->IsTemporarySummon())
            {
                TemporarySummon* pSummon = (TemporarySummon*)cVictim;
                if (pSummon->GetSummonerGuid().IsCreatureOrVehicle())
                    if(Creature* pSummoner = cVictim->GetMap()->GetCreature(pSummon->GetSummonerGuid()))
                        if (pSummoner->AI())
                            pSummoner->AI()->SummonedCreatureJustDied(cVictim);
            }
            else if (pOwner && pOwner->GetTypeId() == TYPEID_UNIT)
            {
                if (((Creature*)pOwner)->AI())
                    ((Creature*)pOwner)->AI()->SummonedCreatureJustDied(cVictim);
            }

            if (InstanceData* mapInstance = cVictim->GetInstanceData())
                mapInstance->OnCreatureDeath(cVictim);

            // Dungeon specific stuff, only applies to players killing creatures
            if(cVictim->GetInstanceId())
            {
                Map *m = cVictim->GetMap();
                Player *creditedPlayer = GetCharmerOrOwnerPlayerOrPlayerItself();
                // TODO: do instance binding anyway if the charmer/owner is offline

                if(m->IsDungeon() && creditedPlayer)
                {
                    if (m->IsRaidOrHeroicDungeon())
                    {
                        if(cVictim->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
                            ((InstanceMap *)m)->PermBindAllPlayers(creditedPlayer);
                    }
                    else
                    {
                        // the reset time is set but not added to the scheduler
                        // until the players leave the instance
                        time_t resettime = cVictim->GetRespawnTimeEx() + 2 * HOUR;
                        if (InstanceSave *save = m->GetInstanceSave())
                            if (save->GetResetTime() < resettime)
                                save->SetResetTime(resettime);
                    }
                }
            }
        }

        // last damage from non duel opponent or opponent controlled creature
        if(duel_hasEnded)
        {
            MANGOS_ASSERT(pVictim->GetTypeId()==TYPEID_PLAYER);
            Player *he = (Player*)pVictim;

            MANGOS_ASSERT(he->duel);

            he->duel->opponent->CombatStopWithPets(true);
            he->CombatStopWithPets(true);

            he->DuelComplete(DUEL_INTERUPTED);
        }

        // battleground things (do this at the end, so the death state flag will be properly set to handle in the bg->handlekill)
        if(pVictim->GetTypeId() == TYPEID_PLAYER && ((Player*)pVictim)->InBattleGround())
        {
            Player *killed = ((Player*)pVictim);
            if(BattleGround *bg = killed->GetBattleGround())
                if(player_tap)
                    bg->HandleKillPlayer(killed, player_tap);
        }
        else if(pVictim->GetTypeId() == TYPEID_UNIT)
        {
            if (player_tap)
                if (BattleGround *bg = player_tap->GetBattleGround())
                    bg->HandleKillUnit((Creature*)pVictim, player_tap);
        }
    }
    else                                                    // if (health <= damage)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"DealDamageAlive");

        if (pVictim->GetTypeId() == TYPEID_PLAYER)
            ((Player*)pVictim)->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED, damage);

        pVictim->ModifyHealth(- (int32)damage);

        if(damagetype != DOT)
        {
            if(!getVictim())
            {
                // if not have main target then attack state with target (including AI call)
                //start melee attacks only after melee hit
                Attack(pVictim,(damagetype == DIRECT_DAMAGE));
            }

            // if damage pVictim call AI reaction
            if(pVictim->GetTypeId()==TYPEID_UNIT && ((Creature*)pVictim)->AI())
                ((Creature*)pVictim)->AI()->AttackedBy(this);
        }

        // polymorphed, hex and other negative transformed cases
        uint32 morphSpell = pVictim->getTransForm();
        if (morphSpell && !IsPositiveSpell(morphSpell))
        {
            if (SpellEntry const* morphEntry = sSpellStore.LookupEntry(morphSpell))
            {
                if (IsSpellHaveAura(morphEntry, SPELL_AURA_MOD_CONFUSE))
                    pVictim->RemoveAurasDueToSpell(morphSpell);
                else if (IsSpellHaveAura(morphEntry, SPELL_AURA_MOD_PACIFY_SILENCE))
                    pVictim->RemoveSpellbyDamageTaken(SPELL_AURA_MOD_PACIFY_SILENCE, damage);
            }
        }

        if(damagetype == DIRECT_DAMAGE || damagetype == SPELL_DIRECT_DAMAGE)
        {
            if (!spellProto || !(spellProto->AuraInterruptFlags&AURA_INTERRUPT_FLAG_DIRECT_DAMAGE))
                pVictim->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_DIRECT_DAMAGE);
        }
        if (pVictim->GetTypeId() != TYPEID_PLAYER)
        {
            if(spellProto && IsDamageToThreatSpell(spellProto))
                pVictim->AddThreat(this, float((damage + (cleanDamage ? cleanDamage->absorb : 0))*2), (cleanDamage && cleanDamage->hitOutCome == MELEE_HIT_CRIT), damageSchoolMask, spellProto);
            else
                pVictim->AddThreat(this, float(damage + (cleanDamage ? cleanDamage->absorb : 0)), (cleanDamage && cleanDamage->hitOutCome == MELEE_HIT_CRIT), damageSchoolMask, spellProto);
        }
        else                                                // victim is a player
        {
            // Rage from damage received
            if(this != pVictim && pVictim->getPowerType() == POWER_RAGE)
            {
                uint32 rage_damage = damage + (cleanDamage ? (cleanDamage->damage + cleanDamage->absorb) : 0);
                ((Player*)pVictim)->RewardRage(rage_damage, 0, false);
            }

            // random durability for items (HIT TAKEN)
            if (roll_chance_f(sWorld.getConfig(CONFIG_FLOAT_RATE_DURABILITY_LOSS_DAMAGE)))
            {
                EquipmentSlots slot = EquipmentSlots(urand(0,EQUIPMENT_SLOT_END-1));
                ((Player*)pVictim)->DurabilityPointLossForEquipSlot(slot);
            }
        }

        if(GetTypeId()==TYPEID_PLAYER)
        {
            // random durability for items (HIT DONE)
            if (roll_chance_f(sWorld.getConfig(CONFIG_FLOAT_RATE_DURABILITY_LOSS_DAMAGE)))
            {
                EquipmentSlots slot = EquipmentSlots(urand(0,EQUIPMENT_SLOT_END-1));
                ((Player*)this)->DurabilityPointLossForEquipSlot(slot);
            }
        }

        // TODO: Store auras by interrupt flag to speed this up.
        SpellAuraHolderMap& vAuras = pVictim->GetSpellAuraHolderMap();
        for (SpellAuraHolderMap::const_iterator i = vAuras.begin(), next; i != vAuras.end(); i = next)
        {
            const SpellEntry *se = i->second->GetSpellProto();
            next = i; ++next;
            if (spellProto && spellProto->Id == se->Id) // Not drop auras added by self
                continue;
            if( se->AuraInterruptFlags & AURA_INTERRUPT_FLAG_DAMAGE )
            {
                bool remove = true;
                if (se->procFlags & (1<<3))
                {
                    if (!roll_chance_i(se->procChance))
                        remove = false;
                }
                if (remove)
                {
                    pVictim->RemoveAurasDueToSpell(i->second->GetId());
                    next = vAuras.begin();
                }
            }
        }

        if (damagetype != NODAMAGE && damage && pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            if( damagetype != DOT )
            {
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                {
                    // skip channeled spell (processed differently below)
                    if (i == CURRENT_CHANNELED_SPELL)
                        continue;

                    if(Spell* spell = pVictim->GetCurrentSpell(CurrentSpellTypes(i)))
                    {
                        if(spell->getState() == SPELL_STATE_PREPARING)
                        {
                            if(spell->m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_ABORT_ON_DMG)
                                pVictim->InterruptSpell(CurrentSpellTypes(i));
                            else
                                spell->Delayed();
                        }
                    }
                }
            }

            if(Spell* spell = pVictim->m_currentSpells[CURRENT_CHANNELED_SPELL])
            {
                if (spell->getState() == SPELL_STATE_CASTING)
                {
                    uint32 channelInterruptFlags = spell->m_spellInfo->ChannelInterruptFlags;
                    if( channelInterruptFlags & CHANNEL_FLAG_DELAY )
                    {
                        if(pVictim!=this)                   //don't shorten the duration of channeling if you damage yourself
                            spell->DelayedChannel();
                    }
                    else if( (channelInterruptFlags & (CHANNEL_FLAG_DAMAGE | CHANNEL_FLAG_DAMAGE2)) )
                    {
                        DETAIL_LOG("Spell %u canceled at damage!",spell->m_spellInfo->Id);
                        pVictim->InterruptSpell(CURRENT_CHANNELED_SPELL);
                    }
                }
                else if (spell->getState() == SPELL_STATE_DELAYED)
                    // break channeled spell in delayed state on damage
                {
                    DETAIL_LOG("Spell %u canceled at damage!",spell->m_spellInfo->Id);
                    pVictim->InterruptSpell(CURRENT_CHANNELED_SPELL);
                }
            }
        }

        // last damage from duel opponent
        if(duel_hasEnded)
        {
            MANGOS_ASSERT(pVictim->GetTypeId()==TYPEID_PLAYER);
            Player *he = (Player*)pVictim;

            MANGOS_ASSERT(he->duel);

            he->SetHealth(1);

            he->duel->opponent->CombatStopWithPets(true);
            he->CombatStopWithPets(true);

            he->CastSpell(he, 7267, true);                  // beg
            he->DuelComplete(DUEL_WON);
        }
    }

    DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE,"DealDamageEnd returned %d damage", damage);

    return damage;
}

struct PetOwnerKilledUnitHelper
{
    explicit PetOwnerKilledUnitHelper(Unit* pVictim) : m_victim(pVictim) {}
    void operator()(Unit* pTarget) const
    {
        if (pTarget->GetTypeId() == TYPEID_UNIT)
        {
            if (((Creature*)pTarget)->AI())
                ((Creature*)pTarget)->AI()->OwnerKilledUnit(m_victim);
        }
    }

    Unit* m_victim;
};

void Unit::PetOwnerKilledUnit(Unit* pVictim)
{
    // for minipet and guardians (including protector)
    CallForAllControlledUnits(PetOwnerKilledUnitHelper(pVictim), CONTROLLED_MINIPET|CONTROLLED_GUARDIANS);
}

void Unit::CastStop(uint32 except_spellid)
{
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
        if (m_currentSpells[i] && m_currentSpells[i]->m_spellInfo->Id!=except_spellid)
            InterruptSpell(CurrentSpellTypes(i),false);
}

void Unit::CastSpell(Unit* Victim, uint32 spellId, bool triggered, Item *castItem, Aura* triggeredByAura, ObjectGuid originalCaster, SpellEntry const* triggeredBy)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

    if(!spellInfo)
    {
        if (triggeredByAura)
            sLog.outError("CastSpell: unknown spell id %i by caster: %s triggered by aura %u (eff %u)", spellId, GetGuidStr().c_str(), triggeredByAura->GetId(), triggeredByAura->GetEffIndex());
        else
            sLog.outError("CastSpell: unknown spell id %i by caster: %s", spellId, GetGuidStr().c_str());
        return;
    }

    CastSpell(Victim, spellInfo, triggered, castItem, triggeredByAura, originalCaster, triggeredBy);
}

void Unit::CastSpell(Unit* Victim, SpellEntry const *spellInfo, bool triggered, Item *castItem, Aura* triggeredByAura, ObjectGuid originalCaster, SpellEntry const* triggeredBy)
{
    if(!spellInfo)
    {
        if (triggeredByAura)
            sLog.outError("CastSpell: unknown spell by caster: %s triggered by aura %u (eff %u)", GetGuidStr().c_str(), triggeredByAura->GetId(), triggeredByAura->GetEffIndex());
        else
            sLog.outError("CastSpell: unknown spell by caster: %s", GetGuidStr().c_str());
        return;
    }

    if (castItem)
        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "WORLD: cast Item spellId - %i", spellInfo->Id);

    if (triggeredByAura)
    {
        if(originalCaster.IsEmpty())
            originalCaster = triggeredByAura->GetCasterGuid();

        triggeredBy = triggeredByAura->GetSpellProto();
    }

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster, triggeredBy);

    SpellCastTargets targets;
    targets.setUnitTarget( Victim );
    spell->m_CastItem = castItem;
    spell->prepare(&targets, triggeredByAura);
}

void Unit::CastCustomSpell(Unit* Victim,uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, bool triggered, Item *castItem, Aura* triggeredByAura, ObjectGuid originalCaster, SpellEntry const* triggeredBy)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

    if(!spellInfo)
    {
        if (triggeredByAura)
            sLog.outError("CastCustomSpell: unknown spell id %i by caster: %s triggered by aura %u (eff %u)", spellId, GetGuidStr().c_str(), triggeredByAura->GetId(), triggeredByAura->GetEffIndex());
        else
            sLog.outError("CastCustomSpell: unknown spell id %i by caster: %s", spellId, GetGuidStr().c_str());
        return;
    }

    CastCustomSpell(Victim, spellInfo, bp0, bp1, bp2, triggered, castItem, triggeredByAura, originalCaster, triggeredBy);
}

void Unit::CastCustomSpell(Unit* Victim, SpellEntry const *spellInfo, int32 const* bp0, int32 const* bp1, int32 const* bp2, bool triggered, Item *castItem, Aura* triggeredByAura, ObjectGuid originalCaster, SpellEntry const* triggeredBy)
{
    if(!spellInfo)
    {
        if (triggeredByAura)
            sLog.outError("CastCustomSpell: unknown spell by caster: %s triggered by aura %u (eff %u)", GetGuidStr().c_str(), triggeredByAura->GetId(), triggeredByAura->GetEffIndex());
        else
            sLog.outError("CastCustomSpell: unknown spell by caster: %s", GetGuidStr().c_str());
        return;
    }

    if(sObjectMgr.IsSpellDisabled(spellInfo->Id))
        return;

    if (castItem)
        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "WORLD: cast Item spellId - %i", spellInfo->Id);

    if (triggeredByAura)
    {
        if(originalCaster.IsEmpty())
            if (triggeredByAura->GetHolder())
            {
                originalCaster = triggeredByAura->GetCasterGuid();
                triggeredBy    = triggeredByAura->GetSpellProto();
            }
            else
            {
                sLog.outError("CastCustomSpell: spell %d by caster: %s triggered by aura without original caster and spellholder (CRUSH THERE!)", spellInfo->Id, GetObjectGuid().GetString().c_str());
                return;
            }
    }

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster, triggeredBy);

    if(bp0)
        spell->m_currentBasePoints[EFFECT_INDEX_0] = *bp0;

    if(bp1)
        spell->m_currentBasePoints[EFFECT_INDEX_1] = *bp1;

    if(bp2)
        spell->m_currentBasePoints[EFFECT_INDEX_2] = *bp2;

    SpellCastTargets targets;
    targets.setUnitTarget( Victim );
    spell->m_CastItem = castItem;
    spell->prepare(&targets, triggeredByAura);
}

// used for scripting
void Unit::CastSpell(float x, float y, float z, uint32 spellId, bool triggered, Item *castItem, Aura* triggeredByAura, ObjectGuid originalCaster, SpellEntry const* triggeredBy)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

    if(!spellInfo)
    {
        if (triggeredByAura)
            sLog.outError("CastSpell(x,y,z): unknown spell id %i by caster: %s triggered by aura %u (eff %u)", spellId, GetGuidStr().c_str(), triggeredByAura->GetId(), triggeredByAura->GetEffIndex());
        else
            sLog.outError("CastSpell(x,y,z): unknown spell id %i by caster: %s", spellId, GetGuidStr().c_str());
        return;
    }

    CastSpell(x, y, z, spellInfo, triggered, castItem, triggeredByAura, originalCaster, triggeredBy);
}

// used for scripting
void Unit::CastSpell(float x, float y, float z, SpellEntry const *spellInfo, bool triggered, Item *castItem, Aura* triggeredByAura, ObjectGuid originalCaster, SpellEntry const* triggeredBy)
{
    if(!spellInfo)
    {
        if (triggeredByAura)
            sLog.outError("CastSpell(x,y,z): unknown spell by caster: %s triggered by aura %u (eff %u)", GetGuidStr().c_str(), triggeredByAura->GetId(), triggeredByAura->GetEffIndex());
        else
            sLog.outError("CastSpell(x,y,z): unknown spell by caster: %s", GetGuidStr().c_str());
        return;
    }
	
    if(sObjectMgr.IsSpellDisabled(spellInfo->Id))
        return;

    if (castItem)
        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "WORLD: cast Item spellId - %i", spellInfo->Id);

    if (triggeredByAura)
    {
        if(originalCaster.IsEmpty())
            originalCaster = triggeredByAura->GetCasterGuid();

        triggeredBy = triggeredByAura->GetSpellProto();
    }

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster, triggeredBy);

    SpellCastTargets targets;
    targets.setDestination(x, y, z);
    spell->m_CastItem = castItem;
    spell->prepare(&targets, triggeredByAura);
}

// Obsolete func need remove, here only for comotability vs another patches
uint32 Unit::SpellNonMeleeDamageLog(Unit *pVictim, uint32 spellID, uint32 damage)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellID);
    SpellNonMeleeDamage damageInfo(this, pVictim, spellInfo->Id, SpellSchoolMask(spellInfo->SchoolMask));
    CalculateSpellDamage(&damageInfo, damage, spellInfo);
    damageInfo.target->CalculateAbsorbResistBlock(this, &damageInfo, spellInfo);
    DealDamageMods(damageInfo.target,damageInfo.damage,&damageInfo.absorb);
    SendSpellNonMeleeDamageLog(&damageInfo);
    DealSpellDamage(&damageInfo, true);
    return damageInfo.damage;
}

void Unit::CalculateSpellDamage(SpellNonMeleeDamage *damageInfo, int32 damage, SpellEntry const *spellInfo, WeaponAttackType attackType)
{
    SpellSchoolMask damageSchoolMask = damageInfo->schoolMask;
    Unit *pVictim = damageInfo->target;

    if (damage < 0)
        return;

    if(!this || !pVictim)
        return;
    if(!this->isAlive() || !pVictim->isAlive())
        return;

    // Check spell crit chance
    bool crit = IsSpellCrit(pVictim, spellInfo, damageSchoolMask, attackType);

    // damage bonus (per damage class)
    switch (spellInfo->DmgClass)
    {
        // Melee and Ranged Spells
        case SPELL_DAMAGE_CLASS_RANGED:
        case SPELL_DAMAGE_CLASS_MELEE:
        {
            //Calculate damage bonus
            damage = MeleeDamageBonusDone(pVictim, damage, attackType, spellInfo, SPELL_DIRECT_DAMAGE);
            damage = pVictim->MeleeDamageBonusTaken(this, damage, attackType, spellInfo, SPELL_DIRECT_DAMAGE);

            // if crit add critical bonus
            if (crit)
            {
                damageInfo->HitInfo|= SPELL_HIT_TYPE_CRIT;
                damage = SpellCriticalDamageBonus(spellInfo, damage, pVictim);
                // Resilience - reduce crit damage
                uint32 redunction_affected_damage = CalcNotIgnoreDamageRedunction(damage,damageSchoolMask);
                if (attackType != RANGED_ATTACK)
                    damage -= pVictim->GetMeleeCritDamageReduction(redunction_affected_damage);
                else
                    damage -= pVictim->GetRangedCritDamageReduction(redunction_affected_damage);
            }
        }
        break;
        // Magical Attacks
        case SPELL_DAMAGE_CLASS_NONE:
        case SPELL_DAMAGE_CLASS_MAGIC:
        {
            // Calculate damage bonus
            damage = SpellDamageBonusDone(pVictim, spellInfo, damage, SPELL_DIRECT_DAMAGE);
            damage = pVictim->SpellDamageBonusTaken(this, spellInfo, damage, SPELL_DIRECT_DAMAGE);

            // If crit add critical bonus
            if (crit)
            {
                damageInfo->HitInfo|= SPELL_HIT_TYPE_CRIT;
                damage = SpellCriticalDamageBonus(spellInfo, damage, pVictim);
                // Resilience - reduce crit damage
                uint32 redunction_affected_damage = CalcNotIgnoreDamageRedunction(damage,damageSchoolMask);
                damage -= pVictim->GetSpellCritDamageReduction(redunction_affected_damage);
            }
        }
        break;
    }

    // only from players
    if (GetTypeId() == TYPEID_PLAYER)
    {
        uint32 redunction_affected_damage = CalcNotIgnoreDamageRedunction(damage,damageSchoolMask);
        damage -= pVictim->GetSpellDamageReduction(redunction_affected_damage);
    }

    // damage mitigation
    if (damage > 0)
    {
        // physical damage => armor
        if (damageSchoolMask & SPELL_SCHOOL_MASK_NORMAL)
        {
            uint32 armor_affected_damage = CalcNotIgnoreDamageRedunction(damage,damageSchoolMask);
            damage = damage - armor_affected_damage + CalcArmorReducedDamage(pVictim, armor_affected_damage);
        }
    }
    else
        damage = 0;
    damageInfo->damage = damage;
}

void Unit::DealSpellDamage(SpellNonMeleeDamage *damageInfo, bool durabilityLoss)
{
    if (!damageInfo)
        return;

    Unit *pVictim = damageInfo->target;

    if(!this || !pVictim)
        return;

    if (!pVictim->isAlive() || pVictim->IsTaxiFlying() || (pVictim->GetTypeId() == TYPEID_UNIT && ((Creature*)pVictim)->IsInEvadeMode()))
        return;

    SpellEntry const *spellProto = sSpellStore.LookupEntry(damageInfo->SpellID);
    if (spellProto == NULL)
    {
        sLog.outError("Unit::DealSpellDamage have wrong damageInfo->SpellID: %u", damageInfo->SpellID);
        return;
    }

    //You don't lose health from damage taken from another player while in a sanctuary
    //You still see it in the combat log though
    if (!IsAllowedDamageInArea(pVictim))
        return;

    // Call default DealDamage (send critical in hit info for threat calculation)
    CleanDamage cleanDamage(0, damageInfo->absorb, BASE_ATTACK, damageInfo->HitInfo & SPELL_HIT_TYPE_CRIT ? MELEE_HIT_CRIT : MELEE_HIT_NORMAL);
    DealDamage(pVictim, damageInfo->damage, &cleanDamage, SPELL_DIRECT_DAMAGE, damageInfo->schoolMask, spellProto, durabilityLoss);
    // Check if effect can trigger anything actually (is this a right ATTR ?)
    if( spellProto->AttributesEx3 & SPELL_ATTR_EX3_UNK16 )
        return;

    bool hasWeaponDmgEffect = false;

    for (uint32 i = 0; i < 3; ++i)
    {
        if (spellProto->Effect[i] == SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL || spellProto->Effect[i] == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE || spellProto->Effect[i] == SPELL_EFFECT_WEAPON_DAMAGE || spellProto->Effect[i] == SPELL_EFFECT_NORMALIZED_WEAPON_DMG)
        {    
            hasWeaponDmgEffect = true;
            break;
        }
    }

    if (!(damageInfo->HitInfo & HITINFO_MISS) && hasWeaponDmgEffect) 
    {
        WeaponAttackType attType = GetWeaponAttackType(spellProto);
        // on weapon hit casts
        if(GetTypeId() == TYPEID_PLAYER && pVictim->isAlive())
            ((Player*)this)->CastItemCombatSpell(pVictim, attType);
    }
}

//TODO for melee need create structure as in
void Unit::CalculateMeleeDamage(Unit *pVictim, uint32 damage, CalcDamageInfo *damageInfo, WeaponAttackType attackType)
{
    damageInfo->attacker         = this;
    damageInfo->target           = pVictim;
    damageInfo->damageSchoolMask = GetMeleeDamageSchoolMask();
    damageInfo->attackType       = attackType;
    damageInfo->damage           = 0;
    damageInfo->cleanDamage      = 0;
    damageInfo->absorb           = 0;
    damageInfo->resist           = 0;
    damageInfo->blocked_amount   = 0;

    damageInfo->TargetState      = 0;
    damageInfo->HitInfo          = 0;
    damageInfo->procAttacker     = PROC_FLAG_NONE;
    damageInfo->procVictim       = PROC_FLAG_NONE;
    damageInfo->procEx           = PROC_EX_NONE;
    damageInfo->hitOutCome       = MELEE_HIT_EVADE;

    if(!this || !pVictim)
        return;
    if(!this->isAlive() || !pVictim->isAlive())
        return;

    // Select HitInfo/procAttacker/procVictim flag based on attack type
    switch (attackType)
    {
        case BASE_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_SUCCESSFUL_MELEE_HIT;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_HIT;
            damageInfo->HitInfo      = HITINFO_NORMALSWING2;
            break;
        case OFF_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_SUCCESSFUL_MELEE_HIT | PROC_FLAG_SUCCESSFUL_OFFHAND_HIT;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_HIT;//|PROC_FLAG_TAKEN_OFFHAND_HIT // not used
            damageInfo->HitInfo = HITINFO_LEFTSWING;
            break;
        case RANGED_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_SUCCESSFUL_RANGED_HIT;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_RANGED_HIT;
            damageInfo->HitInfo = 0x08;// test
            break;
        default:
            break;
    }

    // Physical Immune check
    if (damageInfo->target->IsImmunedToDamage(damageInfo->damageSchoolMask))
    {
        damageInfo->HitInfo       |= HITINFO_NORMALSWING;
        damageInfo->TargetState    = VICTIMSTATE_IS_IMMUNE;

        damageInfo->procEx |=PROC_EX_IMMUNE;
        damageInfo->damage         = 0;
        damageInfo->cleanDamage    = 0;
        return;
    }
    damage += CalculateDamage (damageInfo->attackType, false);
    // Add melee damage bonus
    damage = MeleeDamageBonusDone(damageInfo->target, damage, damageInfo->attackType);
    damage = damageInfo->target->MeleeDamageBonusTaken(this, damage, damageInfo->attackType);
    // Calculate armor reduction

    uint32 armor_affected_damage = CalcNotIgnoreDamageRedunction(damage,damageInfo->damageSchoolMask);
    damageInfo->damage = damage - armor_affected_damage + CalcArmorReducedDamage(damageInfo->target, armor_affected_damage);
    damageInfo->cleanDamage += damage - damageInfo->damage;

    damageInfo->hitOutCome = RollMeleeOutcomeAgainst(damageInfo->target, damageInfo->attackType);

    // Disable parry or dodge for ranged attack
    if (damageInfo->attackType == RANGED_ATTACK)
    {
        if (damageInfo->hitOutCome == MELEE_HIT_PARRY) damageInfo->hitOutCome = MELEE_HIT_NORMAL;
        if (damageInfo->hitOutCome == MELEE_HIT_DODGE) damageInfo->hitOutCome = MELEE_HIT_MISS;
    }

    switch(damageInfo->hitOutCome)
    {
        case MELEE_HIT_EVADE:
        {
            damageInfo->HitInfo    |= HITINFO_MISS|HITINFO_SWINGNOHITSOUND;
            damageInfo->TargetState = VICTIMSTATE_EVADES;

            damageInfo->procEx|=PROC_EX_EVADE;
            damageInfo->damage = 0;
            damageInfo->cleanDamage = 0;
            return;
        }
        case MELEE_HIT_MISS:
        {
            damageInfo->HitInfo    |= HITINFO_MISS;
            damageInfo->TargetState = VICTIMSTATE_NORMAL;

            damageInfo->procEx|=PROC_EX_MISS;
            damageInfo->damage = 0;
            damageInfo->cleanDamage = 0;
            break;
        }
        case MELEE_HIT_NORMAL:
            damageInfo->TargetState = VICTIMSTATE_NORMAL;
            damageInfo->procEx|=PROC_EX_NORMAL_HIT;
            break;
        case MELEE_HIT_CRIT:
        {
            damageInfo->HitInfo     |= HITINFO_CRITICALHIT;
            damageInfo->TargetState  = VICTIMSTATE_NORMAL;

            damageInfo->procEx|=PROC_EX_CRITICAL_HIT;
            // Crit bonus calc
            damageInfo->damage += damageInfo->damage;
            int32 mod=0;
            // Apply SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE or SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE
            if(damageInfo->attackType == RANGED_ATTACK)
                mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
            else
                mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);

            mod += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS, SPELL_SCHOOL_MASK_NORMAL);

            uint32 crTypeMask = damageInfo->target->GetCreatureTypeMask();

            // Increase crit damage from SPELL_AURA_MOD_CRIT_PERCENT_VERSUS
            mod += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_CRIT_PERCENT_VERSUS, crTypeMask);
            if (mod!=0)
                damageInfo->damage = int32((damageInfo->damage) * float((100.0f + mod)/100.0f));

            // Resilience - reduce crit damage
            uint32 redunction_affected_damage = CalcNotIgnoreDamageRedunction(damageInfo->damage,damageInfo->damageSchoolMask);
            uint32 resilienceReduction;
            if (attackType != RANGED_ATTACK)
                resilienceReduction = pVictim->GetMeleeCritDamageReduction(redunction_affected_damage);
            else
                resilienceReduction = pVictim->GetRangedCritDamageReduction(redunction_affected_damage);

            damageInfo->damage      -= resilienceReduction;
            damageInfo->cleanDamage += resilienceReduction;
            break;
        }
        case MELEE_HIT_PARRY:
            damageInfo->TargetState  = VICTIMSTATE_PARRY;
            damageInfo->procEx |= PROC_EX_PARRY;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;

        case MELEE_HIT_DODGE:
            damageInfo->TargetState  = VICTIMSTATE_DODGE;
            damageInfo->procEx|=PROC_EX_DODGE;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;
        case MELEE_HIT_BLOCK:
        {
            damageInfo->TargetState = VICTIMSTATE_NORMAL;
            damageInfo->HitInfo |= HITINFO_BLOCK;
            damageInfo->procEx |= PROC_EX_BLOCK;
            damageInfo->blocked_amount = damageInfo->target->GetShieldBlockValue();

            // Target has a chance to double the blocked amount if it has SPELL_AURA_MOD_BLOCK_CRIT_CHANCE
            if (roll_chance_i(pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_CRIT_CHANCE)))
                damageInfo->blocked_amount *= 2;

            if (damageInfo->blocked_amount >= damageInfo->damage)
            {
                damageInfo->TargetState = VICTIMSTATE_BLOCKS;
                damageInfo->blocked_amount = damageInfo->damage;
                damageInfo->procEx |= PROC_EX_FULL_BLOCK;
            }
            else
                damageInfo->procEx |= PROC_EX_NORMAL_HIT;   // Partial blocks can still cause attacker procs

            damageInfo->damage      -= damageInfo->blocked_amount;
            damageInfo->cleanDamage += damageInfo->blocked_amount;
            break;
        }
        case MELEE_HIT_GLANCING:
        {
            damageInfo->HitInfo |= HITINFO_GLANCING;
            damageInfo->TargetState = VICTIMSTATE_NORMAL;
            damageInfo->procEx |= PROC_EX_NORMAL_HIT;
            float reducePercent = 1.0f;                     //damage factor
            // calculate base values and mods
            float baseLowEnd = 1.3f;
            float baseHighEnd = 1.2f;
            switch(getClass())                              // lowering base values for casters
            {
                case CLASS_SHAMAN:
                case CLASS_PRIEST:
                case CLASS_MAGE:
                case CLASS_WARLOCK:
                case CLASS_DRUID:
                    baseLowEnd  -= 0.7f;
                    baseHighEnd -= 0.3f;
                    break;
            }

            float maxLowEnd = 0.6f;
            switch(getClass())                              // upper for melee classes
            {
                case CLASS_WARRIOR:
                case CLASS_ROGUE:
                    maxLowEnd = 0.91f;                      //If the attacker is a melee class then instead the lower value of 0.91
            }

            // calculate values
            int32 diff = damageInfo->target->GetDefenseSkillValue() - GetWeaponSkillValue(damageInfo->attackType);
            float lowEnd  = baseLowEnd - ( 0.05f * diff );
            float highEnd = baseHighEnd - ( 0.03f * diff );

            // apply max/min bounds
            if ( lowEnd < 0.01f )                           //the low end must not go bellow 0.01f
                lowEnd = 0.01f;
            else if ( lowEnd > maxLowEnd )                  //the smaller value of this and 0.6 is kept as the low end
                lowEnd = maxLowEnd;

            if ( highEnd < 0.2f )                           //high end limits
                highEnd = 0.2f;
            if ( highEnd > 0.99f )
                highEnd = 0.99f;

            if(lowEnd > highEnd)                            // prevent negative range size
                lowEnd = highEnd;

            reducePercent = lowEnd + rand_norm_f() * ( highEnd - lowEnd );

            damageInfo->cleanDamage += damageInfo->damage-uint32(reducePercent *  damageInfo->damage);
            damageInfo->damage   = uint32(reducePercent *  damageInfo->damage);
            break;
        }
        case MELEE_HIT_CRUSHING:
        {
            damageInfo->HitInfo     |= HITINFO_CRUSHING;
            damageInfo->TargetState  = VICTIMSTATE_NORMAL;
            damageInfo->procEx|=PROC_EX_NORMAL_HIT;
            // 150% normal damage
            damageInfo->damage += (damageInfo->damage / 2);
            break;
        }
        default:

            break;
    }

    // only from players
    if (GetTypeId() == TYPEID_PLAYER)
    {
        uint32 redunction_affected_damage = CalcNotIgnoreDamageRedunction(damageInfo->damage,damageInfo->damageSchoolMask);
        uint32 resilienceReduction;
        if (attackType != RANGED_ATTACK)
            resilienceReduction = pVictim->GetMeleeDamageReduction(redunction_affected_damage);
        else
            resilienceReduction = pVictim->GetRangedDamageReduction(redunction_affected_damage);
        damageInfo->damage      -= resilienceReduction;
        damageInfo->cleanDamage += resilienceReduction;
    }

    // Calculate absorb resist
    if(int32(damageInfo->damage) > 0)
    {
        damageInfo->procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

        // Calculate absorb & resists
        uint32 absorb_affected_damage = CalcNotIgnoreAbsorbDamage(damageInfo->damage,damageInfo->damageSchoolMask);
        damageInfo->target->CalculateDamageAbsorbAndResist(this, damageInfo->damageSchoolMask, DIRECT_DAMAGE, absorb_affected_damage, &damageInfo->absorb, &damageInfo->resist, true);
        damageInfo->damage-=damageInfo->absorb + damageInfo->resist;
        if (damageInfo->absorb)
        {
            damageInfo->HitInfo|=HITINFO_ABSORB;
            damageInfo->procEx|=PROC_EX_ABSORB;
        }
        if (damageInfo->resist)
            damageInfo->HitInfo|=HITINFO_RESIST;

    }
    else // Umpossible get negative result but....
        damageInfo->damage = 0;
}

void Unit::DealMeleeDamage(CalcDamageInfo *damageInfo, bool durabilityLoss)
{
    if (damageInfo==0) return;
    Unit *pVictim = damageInfo->target;

    if(!this || !pVictim)
        return;

    if (!pVictim->isAlive() || pVictim->IsTaxiFlying() || (pVictim->GetTypeId() == TYPEID_UNIT && ((Creature*)pVictim)->IsInEvadeMode()))
        return;

    //You don't lose health from damage taken from another player while in a sanctuary
    //You still see it in the combat log though
    if (!IsAllowedDamageInArea(pVictim))
        return;

    // Hmmmm dont like this emotes client must by self do all animations
    if (damageInfo->HitInfo&HITINFO_CRITICALHIT)
        pVictim->HandleEmoteCommand(EMOTE_ONESHOT_WOUNDCRITICAL);
    if (damageInfo->blocked_amount && damageInfo->TargetState!=VICTIMSTATE_BLOCKS)
        pVictim->HandleEmoteCommand(EMOTE_ONESHOT_PARRYSHIELD);

    if(damageInfo->TargetState == VICTIMSTATE_PARRY)
    {
        // Get attack timers
        float offtime  = float(pVictim->getAttackTimer(OFF_ATTACK));
        float basetime = float(pVictim->getAttackTimer(BASE_ATTACK));
        // Reduce attack time
        if (pVictim->haveOffhandWeapon() && offtime < basetime)
        {
            float percent20 = pVictim->GetAttackTime(OFF_ATTACK) * 0.20f;
            float percent60 = 3.0f * percent20;
            if(offtime > percent20 && offtime <= percent60)
            {
                pVictim->setAttackTimer(OFF_ATTACK, uint32(percent20));
            }
            else if(offtime > percent60)
            {
                offtime -= 2.0f * percent20;
                pVictim->setAttackTimer(OFF_ATTACK, uint32(offtime));
            }
        }
        else
        {
            float percent20 = pVictim->GetAttackTime(BASE_ATTACK) * 0.20f;
            float percent60 = 3.0f * percent20;
            if(basetime > percent20 && basetime <= percent60)
            {
                pVictim->setAttackTimer(BASE_ATTACK, uint32(percent20));
            }
            else if(basetime > percent60)
            {
                basetime -= 2.0f * percent20;
                pVictim->setAttackTimer(BASE_ATTACK, uint32(basetime));
            }
        }
    }

    // Call default DealDamage
    CleanDamage cleanDamage(damageInfo->cleanDamage, damageInfo->absorb, damageInfo->attackType, damageInfo->hitOutCome);
    DealDamage(pVictim, damageInfo->damage, &cleanDamage, DIRECT_DAMAGE, damageInfo->damageSchoolMask, NULL, durabilityLoss);

    // If this is a creature and it attacks from behind it has a probability to daze it's victim
    if( (damageInfo->hitOutCome==MELEE_HIT_CRIT || damageInfo->hitOutCome==MELEE_HIT_CRUSHING || damageInfo->hitOutCome==MELEE_HIT_NORMAL || damageInfo->hitOutCome==MELEE_HIT_GLANCING) &&
        GetTypeId() != TYPEID_PLAYER && ((Creature*)this)->GetCharmerOrOwnerGuid().IsEmpty() && !pVictim->HasInArc(M_PI_F, this) )
    {
        // -probability is between 0% and 40%
        // 20% base chance
        float Probability = 20.0f;

        //there is a newbie protection, at level 10 just 7% base chance; assuming linear function
        if( pVictim->getLevel() < 30 )
            Probability = 0.65f*pVictim->getLevel()+0.5f;

        uint32 VictimDefense=pVictim->GetDefenseSkillValue();
        uint32 AttackerMeleeSkill=GetUnitMeleeSkill();

        Probability *= AttackerMeleeSkill/(float)VictimDefense;

        if(Probability > 40.0f)
            Probability = 40.0f;

        if(roll_chance_f(Probability))
            CastSpell(pVictim, 1604, true);
    }

    // If not miss
    if (!(damageInfo->HitInfo & HITINFO_MISS))
    {
        // on weapon hit casts
        if(GetTypeId() == TYPEID_PLAYER && pVictim->isAlive())
            ((Player*)this)->CastItemCombatSpell(pVictim, damageInfo->attackType);

        // victim's damage shield
        std::set<Aura*> alreadyDone;
        AuraList const& vDamageShields = pVictim->GetAurasByType(SPELL_AURA_DAMAGE_SHIELD);
        for(AuraList::const_iterator i = vDamageShields.begin(); i != vDamageShields.end();)
        {
            if (alreadyDone.find(*i) == alreadyDone.end())
            {
                alreadyDone.insert(*i);
                uint32 damage=(*i)->GetModifier()->m_amount;
                SpellEntry const *i_spellProto = (*i)->GetSpellProto();

                // Thorns
                if (i_spellProto->SpellFamilyName == SPELLFAMILY_DRUID && i_spellProto->SpellFamilyFlags & UI64LIT(0x00000100))
                {
                    Unit::AuraList const& dummyList = pVictim->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator iter = dummyList.begin(); iter != dummyList.end(); ++iter)
                    {
                        // Brambles
                        if((*iter)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID &&
                            (*iter)->GetSpellProto()->SpellIconID == 53)
                        {
                            damage += uint32(damage * (*iter)->GetModifier()->m_amount / 100);
                            break;
                        }
                    }
                }

                //Calculate absorb resist ??? no data in opcode for this possibly unable to absorb or resist?
                //uint32 absorb;
                //uint32 resist;
                //CalcAbsorbResist(pVictim, SpellSchools(spellProto->School), SPELL_DIRECT_DAMAGE, damage, &absorb, &resist);
                //damage-=absorb + resist;

                pVictim->DealDamageMods(this,damage,NULL);

                uint32 targetHealth = GetHealth();
                uint32 overkill = damage > targetHealth ? damage - targetHealth : 0;

                WorldPacket data(SMSG_SPELLDAMAGESHIELD,(8+8+4+4+4+4));
                data << pVictim->GetObjectGuid();
                data << GetObjectGuid();
                data << uint32(i_spellProto->Id);
                data << uint32(damage);                  // Damage
                data << uint32(overkill);                // Overkill
                data << uint32(i_spellProto->SchoolMask);
                pVictim->SendMessageToSet(&data, true );

                pVictim->DealDamage(this, damage, 0, SPELL_DIRECT_DAMAGE, GetSpellSchoolMask(i_spellProto), i_spellProto, true);

                i = vDamageShields.begin();
            }
            else
                ++i;
        }
    }
}


void Unit::HandleEmoteCommand(uint32 emote_id)
{
    WorldPacket data( SMSG_EMOTE, 4 + 8 );
    data << uint32(emote_id);
    data << GetObjectGuid();
    SendMessageToSet(&data, true);
}

void Unit::HandleEmoteState(uint32 emote_id)
{
    SetUInt32Value(UNIT_NPC_EMOTESTATE, emote_id);
}

void Unit::HandleEmote(uint32 emote_id)
{
    if (!emote_id)
        HandleEmoteState(0);
    else if (EmotesEntry const* emoteEntry = sEmotesStore.LookupEntry(emote_id))
    {
        if (emoteEntry->EmoteType)                          // 1,2 states, 0 command
            HandleEmoteState(emote_id);
        else
            HandleEmoteCommand(emote_id);
    }
}

uint32 Unit::CalcNotIgnoreAbsorbDamage( uint32 damage, SpellSchoolMask damageSchoolMask, SpellEntry const* spellInfo /*= NULL*/)
{
    float absorb_affected_rate = 1.0f;
    Unit::AuraList const& ignoreAbsorbSchool = GetAurasByType(SPELL_AURA_MOD_IGNORE_ABSORB_SCHOOL);
    for(Unit::AuraList::const_iterator i = ignoreAbsorbSchool.begin(); i != ignoreAbsorbSchool.end(); ++i)
        if ((*i)->GetMiscValue() & damageSchoolMask)
            absorb_affected_rate *= (100.0f - (*i)->GetModifier()->m_amount)/100.0f;

    if(spellInfo)
    {
        Unit::AuraList const& ignoreAbsorbForSpell = GetAurasByType(SPELL_AURA_MOD_IGNORE_ABSORB_FOR_SPELL);
        for(Unit::AuraList::const_iterator citr = ignoreAbsorbForSpell.begin(); citr != ignoreAbsorbForSpell.end(); ++citr)
            if ((*citr)->isAffectedOnSpell(spellInfo))
                absorb_affected_rate *= (100.0f - (*citr)->GetModifier()->m_amount)/100.0f;
    }

    return absorb_affected_rate <= 0.0f ? 0 : (absorb_affected_rate < 1.0f  ? uint32(damage * absorb_affected_rate) : damage);
}

uint32 Unit::CalcNotIgnoreDamageRedunction( uint32 damage, SpellSchoolMask damageSchoolMask)
{
    float absorb_affected_rate = 1.0f;
    Unit::AuraList const& ignoreAbsorb = GetAurasByType(SPELL_AURA_MOD_IGNORE_DAMAGE_REDUCTION_SCHOOL);
    for(Unit::AuraList::const_iterator i = ignoreAbsorb.begin(); i != ignoreAbsorb.end(); ++i)
        if ((*i)->GetMiscValue() & damageSchoolMask)
            absorb_affected_rate *= (100.0f - (*i)->GetModifier()->m_amount)/100.0f;

    return absorb_affected_rate <= 0.0f ? 0 : (absorb_affected_rate < 1.0f  ? uint32(damage * absorb_affected_rate) : damage);
}

uint32 Unit::CalcArmorReducedDamage(Unit* pVictim, const uint32 damage)
{
    uint32 newdamage = 0;
    float armor = (float)pVictim->GetArmor();

    // Ignore enemy armor by SPELL_AURA_MOD_TARGET_RESISTANCE aura
    armor += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_TARGET_RESISTANCE, SPELL_SCHOOL_MASK_NORMAL);

    // Apply Player CR_ARMOR_PENETRATION rating and percent talents
    if (GetTypeId()==TYPEID_PLAYER)
    {
        float maxArmorPen = 400 + 85 * pVictim->getLevel();
        if (getLevel() > 59)
            maxArmorPen += 4.5f * 85 * (pVictim->getLevel()-59);
        // Cap ignored armor to this value
        maxArmorPen = std::min(((armor+maxArmorPen)/3), armor);
        // Also, armor penetration is limited to 100% since 3.1.2, before greater values did
        // continue to give benefit for targets with more armor than the above cap
        float armorPenPct = std::min(100.f, ((Player*)this)->GetArmorPenetrationPct());
        armor -= maxArmorPen * armorPenPct / 100.0f;
    }

    if (armor < 0.0f)
        armor = 0.0f;

    float levelModifier = (float)getLevel();
    if (levelModifier > 59)
        levelModifier = levelModifier + (4.5f * (levelModifier-59));

    float tmpvalue = 0.1f * armor / (8.5f * levelModifier + 40);
    tmpvalue = tmpvalue/(1.0f + tmpvalue);

    if (tmpvalue < 0.0f)
        tmpvalue = 0.0f;
    if (tmpvalue > 0.75f)
        tmpvalue = 0.75f;

    newdamage = uint32(damage - (damage * tmpvalue));

    return (newdamage > 1) ? newdamage : 1;
}

void Unit::CalculateDamageAbsorbAndResist(Unit *pCaster, SpellSchoolMask schoolMask, DamageEffectType damagetype, const uint32 damage, uint32 *absorb, uint32 *resist, bool canReflect)
{
    if(!pCaster || !isAlive() || !damage)
        return;

    // Magic damage, check for resists
    if ((schoolMask & SPELL_SCHOOL_MASK_NORMAL)==0)
    {
        // Get base victim resistance for school
        float tmpvalue2 = (float)GetResistance(GetFirstSchoolInMask(schoolMask));
        // Ignore resistance by self SPELL_AURA_MOD_TARGET_RESISTANCE aura
        tmpvalue2 += (float)pCaster->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_TARGET_RESISTANCE, schoolMask);

        if (pCaster->GetTypeId() == TYPEID_PLAYER)
            tmpvalue2 -= (float)((Player*)pCaster)->GetSpellPenetrationItemMod();

        tmpvalue2 *= (float)(0.15f / getLevel());
        if (tmpvalue2 < 0.0f)
            tmpvalue2 = 0.0f;
        if (tmpvalue2 > 0.75f)
            tmpvalue2 = 0.75f;
        uint32 ran = urand(0, 100);
        float faq[4] = {24.0f,6.0f,4.0f,6.0f};
        uint8 m = 0;
        float Binom = 0.0f;
        for (uint8 i = 0; i < 4; ++i)
        {
            Binom += 2400 *( powf(tmpvalue2, float(i)) * powf( (1-tmpvalue2), float(4-i)))/faq[i];
            if (ran > Binom )
                ++m;
            else
                break;
        }
        if (damagetype == DOT && m == 4)
            *resist += uint32(damage - 1);
        else
            *resist += uint32(damage * m / 4);
        if(*resist > damage)
            *resist = damage;
    }
    else
        *resist = 0;

    int32 RemainingDamage = damage - *resist;

    // Get unit state (need for some absorb check)
    uint32 unitflag = GetUInt32Value(UNIT_FIELD_FLAGS);
    // Reflect damage spells (not cast any damage spell in aura lookup)
    uint32 reflectSpell = 0;
    int32  reflectDamage = 0;
    Aura*  reflectTriggeredBy = NULL;                       // expected as not expired at reflect as in current cases
    // Death Prevention Aura
    SpellEntry const*  preventDeathSpell = NULL;
    int32  preventDeathAmount = 0;

    // full absorb cases (by chance)
    AuraList const& vAbsorb = GetAurasByType(SPELL_AURA_SCHOOL_ABSORB);
    for(AuraList::const_iterator i = vAbsorb.begin(); i != vAbsorb.end() && RemainingDamage > 0; ++i)
    {
        // only work with proper school mask damage
        Modifier* i_mod = (*i)->GetModifier();
        if (!(i_mod->m_miscvalue & schoolMask))
            continue;

        SpellEntry const* i_spellProto = (*i)->GetSpellProto();
        // Fire Ward or Frost Ward
        if(i_spellProto->SpellFamilyName == SPELLFAMILY_MAGE && i_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000108))
        {
            int chance = 0;
            Unit::AuraList const& auras = GetAurasByType(SPELL_AURA_ADD_PCT_MODIFIER);
            for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                SpellEntry const* itr_spellProto = (*itr)->GetSpellProto();
                // Frost Warding (chance full absorb)
                if (itr_spellProto->SpellFamilyName == SPELLFAMILY_MAGE && itr_spellProto->SpellIconID == 501)
                {
                    // chance stored in next dummy effect
                    chance = itr_spellProto->CalculateSimpleValue(EFFECT_INDEX_1);
                    break;
                }
            }
            if(roll_chance_i(chance))
            {
                int32 amount = RemainingDamage;
                RemainingDamage = 0;

                // Frost Warding (mana regen)
                CastCustomSpell(this, 57776, &amount, NULL, NULL, true, NULL, *i);
                break;
            }
        }
    }

    // Need remove expired auras after
    bool existExpired = false;

    // Incanter's Absorption, for converting to spell power
    int32 incanterAbsorption = 0;

    // absorb without mana cost
    AuraList const& vSchoolAbsorb = GetAurasByType(SPELL_AURA_SCHOOL_ABSORB);
    for(AuraList::const_iterator i = vSchoolAbsorb.begin(); i != vSchoolAbsorb.end() && RemainingDamage > 0; ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (!(mod->m_miscvalue & schoolMask))
            continue;

        SpellEntry const* spellProto = (*i)->GetSpellProto();

        // Max Amount can be absorbed by this aura
        int32  currentAbsorb = mod->m_amount;

        // Found empty aura (impossible but..)
        if (currentAbsorb <=0)
        {
            existExpired = true;
            continue;
        }

        // Handle custom absorb auras
        // TODO: try find better way

        switch(spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                // Astral Shift
                if (spellProto->SpellIconID == 3066)
                {
                    //reduces all damage taken while stun, fear or silence
                    if (unitflag & (UNIT_FLAG_STUNNED|UNIT_FLAG_FLEEING|UNIT_FLAG_SILENCED))
                        RemainingDamage -= RemainingDamage * currentAbsorb / 100;
                    continue;
                }
                // Nerves of Steel
                if (spellProto->SpellIconID == 2115)
                {
                    // while affected by Stun and Fear
                    if (unitflag&(UNIT_FLAG_STUNNED|UNIT_FLAG_FLEEING))
                        RemainingDamage -= RemainingDamage * currentAbsorb / 100;
                    continue;
                }
                // Spell Deflection
                if (spellProto->SpellIconID == 3006)
                {
                    // You have a chance equal to your Parry chance
                    if (damagetype == SPELL_DIRECT_DAMAGE &&             // Only for direct spell damage
                        roll_chance_f(GetUnitParryChance()))             // Roll chance
                        RemainingDamage -= RemainingDamage * currentAbsorb / 100;
                    continue;
                }
                // Reflective Shield (Lady Malande boss)
                if (spellProto->Id == 41475 && canReflect)
                {
                    if(RemainingDamage < currentAbsorb)
                        reflectDamage = RemainingDamage / 2;
                    else
                        reflectDamage = currentAbsorb / 2;
                    reflectSpell = 33619;
                    reflectTriggeredBy = *i;
                    reflectTriggeredBy->SetInUse(true);     // lock aura from final deletion until processing
                    break;
                }
                if (spellProto->Id == 39228 || // Argussian Compass
                    spellProto->Id == 60218)   // Essence of Gossamer
                {
                    // Max absorb stored in 1 dummy effect
                    int32 max_absorb = spellProto->CalculateSimpleValue(EFFECT_INDEX_1);
                    if (max_absorb < currentAbsorb)
                        currentAbsorb = max_absorb;
                    break;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                // Primal Tenacity
                if (spellProto->SpellIconID == 2253)
                {
                    //reduces all damage taken while Stunned and in Cat Form
                    if (GetShapeshiftForm() == FORM_CAT && (unitflag & UNIT_FLAG_STUNNED))
                        RemainingDamage -= RemainingDamage * currentAbsorb / 100;
                    continue;
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Cheat Death (make less prio with Guardian Spirit case)
                if (spellProto->SpellIconID == 2109)
                {
                    if (!preventDeathSpell &&
                        GetTypeId()==TYPEID_PLAYER &&           // Only players
                        !((Player*)this)->HasSpellCooldown(31231) &&
                                                                // Only if no cooldown
                        roll_chance_i((*i)->GetModifier()->m_amount))
                                                                // Only if roll
                    {
                        preventDeathSpell = (*i)->GetSpellProto();
                    }
                    // always skip this spell in charge dropping, absorb amount calculation since it has chance as m_amount and doesn't need to absorb any damage
                    continue;
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                // Ardent Defender
                if (spellProto->SpellIconID == 2135 && GetTypeId() == TYPEID_PLAYER)
                {
                    int32 remainingHealth = GetHealth() - RemainingDamage;
                    uint32 allowedHealth = GetMaxHealth() * 0.35f;
                    // If damage kills us
                    if (remainingHealth <= 0 && !HasAura(66233))
                    {
                        // Cast healing spell, completely avoid damage
                        RemainingDamage = 0;

                        uint32 defenseSkillValue = GetDefenseSkillValue();
                        // Max heal when defense skill denies critical hits from raid bosses
                        // Formula: max defense at level + 140 (raiting from gear)
                        uint32 reqDefForMaxHeal  = getLevel() * 5 + 140;
                        float pctFromDefense = (defenseSkillValue >= reqDefForMaxHeal)
                            ? 1.0f
                            : float(defenseSkillValue) / float(reqDefForMaxHeal);

                        int32 healAmount = GetMaxHealth() * ((*i)->GetSpellProto()->EffectBasePoints[1] + 1) / 100.0f * pctFromDefense;
                        CastSpell(this, 66233, true);
                        CastCustomSpell(this, 66235, &healAmount, NULL, NULL, true);
                    }
                    else if (remainingHealth < int32(allowedHealth))
                    {
                        // Reduce damage that brings us under 35% (or full damage if we are already under 35%) by x%
                        uint32 damageToReduce = (GetHealth() < allowedHealth)
                            ? RemainingDamage
                            : allowedHealth - remainingHealth;
                        RemainingDamage -= damageToReduce * currentAbsorb / 100;
                    }
                    continue;
                }
                break;
            }
            case SPELLFAMILY_PRIEST:
            {
                // Guardian Spirit
                if (spellProto->SpellIconID == 2873)
                {
                    preventDeathSpell = (*i)->GetSpellProto();
                    preventDeathAmount = (*i)->GetModifier()->m_amount;
                    continue;
                }
                // Reflective Shield
                if (spellProto->SpellFamilyFlags == 0x1 && canReflect)
                {
                    if (pCaster == this)
                        break;
                    Unit* caster = (*i)->GetCaster();
                    if (!caster)
                        break;
                    AuraList const& vOverRideCS = caster->GetAurasByType(SPELL_AURA_DUMMY);
                    for(AuraList::const_iterator k = vOverRideCS.begin(); k != vOverRideCS.end(); ++k)
                    {
                        switch((*k)->GetModifier()->m_miscvalue)
                        {
                            case 5065:                      // Rank 1
                            case 5064:                      // Rank 2
                            {
                                if(RemainingDamage >= currentAbsorb)
                                    reflectDamage = (*k)->GetModifier()->m_amount * currentAbsorb/100;
                                else
                                    reflectDamage = (*k)->GetModifier()->m_amount * RemainingDamage/100;
                                reflectSpell = 33619;
                                reflectTriggeredBy = *i;
                                reflectTriggeredBy->SetInUse(true);// lock aura from final deletion until processing
                            } break;
                            default: break;
                        }
                    }
                    break;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                // Astral Shift
                if (spellProto->SpellIconID == 3066)
                {
                    //reduces all damage taken while stun, fear or silence
                    if (unitflag & (UNIT_FLAG_STUNNED|UNIT_FLAG_FLEEING|UNIT_FLAG_SILENCED))
                        RemainingDamage -= RemainingDamage * currentAbsorb / 100;
                    continue;
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                // Shadow of Death
                if (spellProto->SpellIconID == 1958)
                {
                    // TODO: absorb only while transform
                    continue;
                }
                // Anti-Magic Shell (on self)
                if (spellProto->Id == 48707)
                {
                    // damage absorbed by Anti-Magic Shell energizes the DK with additional runic power.
                    // This, if I'm not mistaken, shows that we get back ~2% of the absorbed damage as runic power.
                    int32 absorbed = RemainingDamage * currentAbsorb / 100;
                    int32 regen = absorbed * 2 / 10;
                    CastCustomSpell(this, 49088, &regen, NULL, NULL, true, NULL, *i);
                    RemainingDamage -= absorbed;
                    continue;
                }
                // Anti-Magic Shell (on single party/raid member)
                if (spellProto->Id == 50462)
                {
                    RemainingDamage -= RemainingDamage * currentAbsorb / 100;
                    continue;
                }
                // Unbreakable armor
                if (spellProto->Id == 51271)
                {
                    int32 absorbed = GetArmor() * currentAbsorb / 100;
                    // If we have a glyph
                    if (Aura* aur = GetDummyAura(58635))
                        absorbed += absorbed * aur->GetModifier()->m_amount / 100;
                    RemainingDamage = (RemainingDamage < absorbed) ? 0 : RemainingDamage - absorbed;
                    continue;
                }
                // Anti-Magic Zone
                if (spellProto->Id == 50461)
                {
                    Unit* caster = (*i)->GetCaster();
                    if (!caster)
                        continue;
                    int32 absorbed = RemainingDamage * currentAbsorb / 100;
                    int32 canabsorb = caster->GetHealth();
                    if (canabsorb < absorbed)
                        absorbed = canabsorb;

                    RemainingDamage -= absorbed;

                    uint32 ab_damage = absorbed;
                    pCaster->DealDamageMods(caster,ab_damage,NULL);
                    pCaster->DealDamage(caster, ab_damage, NULL, damagetype, schoolMask, 0, false);
                    continue;
                }
                // Will of Necropolis
                if (spellProto->SpellIconID == 857)
                {
                    // Apply absorb only on damage below 35% hp
                    int32 absorbableDamage = RemainingDamage + 0.35f * GetMaxHealth() - GetHealth();
                    if (absorbableDamage > RemainingDamage)
                        absorbableDamage = RemainingDamage;
                    if (absorbableDamage > 0)
                        RemainingDamage -= absorbableDamage * currentAbsorb / 100;
                    continue;
                }      
                break;
            }
            default:
                break;
        }

        // currentAbsorb - damage can be absorbed by shield
        // If need absorb less damage
        if (RemainingDamage < currentAbsorb)
            currentAbsorb = RemainingDamage;

        RemainingDamage -= currentAbsorb;

        // Fire Ward or Frost Ward or Ice Barrier (or Mana Shield)
        // for Incanter's Absorption converting to spell power
        if (spellProto->SpellFamilyName == SPELLFAMILY_MAGE && spellProto->SpellFamilyFlags2 & 0x000008)
            incanterAbsorption += currentAbsorb;

        // Reduce shield amount
        mod->m_amount-=currentAbsorb;
        if((*i)->GetHolder()->DropAuraCharge())
            mod->m_amount = 0;
        // Need remove it later
        if (mod->m_amount<=0)
            existExpired = true;
    }

    // Remove all expired absorb auras
    if (existExpired)
    {
        for(AuraList::const_iterator i = vSchoolAbsorb.begin(); i != vSchoolAbsorb.end();)
        {
            if ((*i)->GetModifier()->m_amount<=0)
            {
                RemoveAurasDueToSpell((*i)->GetId(), NULL, AURA_REMOVE_BY_SHIELD_BREAK);
                i = vSchoolAbsorb.begin();
            }
            else
                ++i;
        }
    }

    // Cast back reflect damage spell
    if (canReflect && reflectSpell)
    {
        CastCustomSpell(pCaster,  reflectSpell, &reflectDamage, NULL, NULL, true, NULL, reflectTriggeredBy);
        reflectTriggeredBy->SetInUse(false);                // free lock from deletion
    }


    // absorb by mana cost
    AuraList const& vManaShield = GetAurasByType(SPELL_AURA_MANA_SHIELD);
    for(AuraList::const_iterator i = vManaShield.begin(), next; i != vManaShield.end() && RemainingDamage > 0; i = next)
    {
        next = i; ++next;

        // check damage school mask
        if(((*i)->GetModifier()->m_miscvalue & schoolMask)==0)
            continue;

        int32 currentAbsorb;
        if (RemainingDamage >= (*i)->GetModifier()->m_amount)
            currentAbsorb = (*i)->GetModifier()->m_amount;
        else
            currentAbsorb = RemainingDamage;

        if (float manaMultiplier = (*i)->GetSpellProto()->EffectMultipleValue[(*i)->GetEffIndex()])
        {
            if(Player *modOwner = GetSpellModOwner())
                modOwner->ApplySpellMod((*i)->GetId(), SPELLMOD_MULTIPLE_VALUE, manaMultiplier);

            int32 maxAbsorb = int32(GetPower(POWER_MANA) / manaMultiplier);
            if (currentAbsorb > maxAbsorb)
                currentAbsorb = maxAbsorb;

            int32 manaReduction = int32(currentAbsorb * manaMultiplier);
            ApplyPowerMod(POWER_MANA, manaReduction, false);
        }

        // Mana Shield (or Fire Ward or Frost Ward or Ice Barrier)
        // for Incanter's Absorption converting to spell power
        if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_MAGE && (*i)->GetSpellProto()->SpellFamilyFlags2 & 0x000008)
            incanterAbsorption += currentAbsorb;

        (*i)->GetModifier()->m_amount -= currentAbsorb;
        if((*i)->GetModifier()->m_amount <= 0)
        {
            RemoveAurasDueToSpell((*i)->GetId());
            next = vManaShield.begin();
        }

        RemainingDamage -= currentAbsorb;
    }

    // effects dependent from full absorb amount
    // Incanter's Absorption, if have affective absorbing
    if (incanterAbsorption)
    {
        Unit::AuraList const& auras = GetAurasByType(SPELL_AURA_DUMMY);
        for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        {
            SpellEntry const* itr_spellProto = (*itr)->GetSpellProto();

            // Incanter's Absorption
            if (itr_spellProto->SpellFamilyName == SPELLFAMILY_GENERIC &&
                itr_spellProto->SpellIconID == 2941)
            {
                int32 amount = int32(incanterAbsorption * (*itr)->GetModifier()->m_amount / 100);

                // apply normalized part of already accumulated amount in aura
                if (Aura* spdAura = GetAura(44413, EFFECT_INDEX_0))
                    amount += spdAura->GetModifier()->m_amount * spdAura->GetAuraDuration() / spdAura->GetAuraMaxDuration();

                // Incanter's Absorption (triggered absorb based spell power, will replace existing if any)
                CastCustomSpell(this, 44413, &amount, NULL, NULL, true);
                break;
            }
        }
    }

    // only split damage if not damaging yourself
    if(pCaster != this)
    {
        AuraList const& vSplitDamageFlat = GetAurasByType(SPELL_AURA_SPLIT_DAMAGE_FLAT);
        for(AuraList::const_iterator i = vSplitDamageFlat.begin(), next; i != vSplitDamageFlat.end() && RemainingDamage >= 0; i = next)
        {
            next = i; ++next;

            // check damage school mask
            if(((*i)->GetModifier()->m_miscvalue & schoolMask)==0)
                continue;

            // Damage can be splitted only if aura has an alive caster
            Unit *caster = (*i)->GetCaster();
            if(!caster || caster == this || !caster->IsInWorld() || !caster->isAlive())
                continue;

            int32 currentAbsorb;
            if (RemainingDamage >= (*i)->GetModifier()->m_amount)
                currentAbsorb = (*i)->GetModifier()->m_amount;
            else
                currentAbsorb = RemainingDamage;

            RemainingDamage -= currentAbsorb;


            uint32 splitted = currentAbsorb;
            uint32 splitted_absorb = 0;
            pCaster->DealDamageMods(caster,splitted,&splitted_absorb);

            pCaster->SendSpellNonMeleeDamageLog(caster, (*i)->GetSpellProto()->Id, splitted, schoolMask, splitted_absorb, 0, false, 0, false);

            CleanDamage cleanDamage = CleanDamage(splitted, 0, BASE_ATTACK, MELEE_HIT_NORMAL);
            pCaster->DealDamage(caster, splitted, &cleanDamage, DIRECT_DAMAGE, schoolMask, (*i)->GetSpellProto(), false);
        }

        AuraList const& vSplitDamagePct = GetAurasByType(SPELL_AURA_SPLIT_DAMAGE_PCT);
        for(AuraList::const_iterator i = vSplitDamagePct.begin(), next; i != vSplitDamagePct.end() && RemainingDamage >= 0; i = next)
        {
            next = i; ++next;

            // check damage school mask
            if(((*i)->GetModifier()->m_miscvalue & schoolMask)==0)
                continue;

            // Damage can be splitted only if aura has an alive caster
            Unit *caster = (*i)->GetCaster();
            if(!caster || caster == this || !caster->IsInWorld() || !caster->isAlive())
                continue;

            uint32 splitted = uint32(RemainingDamage * (*i)->GetModifier()->m_amount / 100.0f);

            RemainingDamage -=  int32(splitted);

            uint32 split_absorb = 0;
            pCaster->DealDamageMods(caster,splitted,&split_absorb);

            pCaster->SendSpellNonMeleeDamageLog(caster, (*i)->GetSpellProto()->Id, splitted, schoolMask, split_absorb, 0, false, 0, false);

            CleanDamage cleanDamage = CleanDamage(splitted, 0, BASE_ATTACK, MELEE_HIT_NORMAL);
            pCaster->DealDamage(caster, splitted, &cleanDamage, DIRECT_DAMAGE, schoolMask, (*i)->GetSpellProto(), false);
        }
    }

    // Apply death prevention spells effects
    if (preventDeathSpell && RemainingDamage >= (int32)GetHealth())
    {
        switch(preventDeathSpell->SpellFamilyName)
        {
            // Cheat Death
            case SPELLFAMILY_ROGUE:
            {
                // Cheat Death
                if (preventDeathSpell->SpellIconID == 2109)
                {
                    CastSpell(this,31231,true);
                    ((Player*)this)->AddSpellCooldown(31231,0,time(NULL)+60);
                    // with health > 10% lost health until health==10%, in other case no losses
                    uint32 health10 = GetMaxHealth()/10;
                    RemainingDamage = GetHealth() > health10 ? GetHealth() - health10 : 0;
                }
                break;
            }
            // Guardian Spirit
            case SPELLFAMILY_PRIEST:
            {
                // Guardian Spirit
                if (preventDeathSpell->SpellIconID == 2873)
                {
                    int32 healAmount = GetMaxHealth() * preventDeathAmount / 100;
                    CastCustomSpell(this, 48153, &healAmount, NULL, NULL, true);
                    RemoveAurasDueToSpell(preventDeathSpell->Id);
                    RemainingDamage = 0;
                }
                break;
            }
        }
    }

    *absorb = damage - RemainingDamage - *resist;
}

void Unit::CalculateAbsorbResistBlock(Unit *pCaster, SpellNonMeleeDamage *damageInfo, SpellEntry const* spellProto, WeaponAttackType attType)
{
    bool blocked = false;
    // Get blocked status
    switch (spellProto->DmgClass)
    {
        // Melee and Ranged Spells
        case SPELL_DAMAGE_CLASS_RANGED:
        case SPELL_DAMAGE_CLASS_MELEE:
            blocked = IsSpellBlocked(pCaster, spellProto, attType);
            break;
        default:
            break;
    }

    if (blocked)
    {
        damageInfo->blocked = GetShieldBlockValue();
        if (damageInfo->damage < damageInfo->blocked)
            damageInfo->blocked = damageInfo->damage;
        damageInfo->damage-=damageInfo->blocked;
    }

    uint32 absorb_affected_damage = pCaster->CalcNotIgnoreAbsorbDamage(damageInfo->damage,GetSpellSchoolMask(spellProto),spellProto);
    CalculateDamageAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), SPELL_DIRECT_DAMAGE, absorb_affected_damage, &damageInfo->absorb, &damageInfo->resist, !(spellProto->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));
    damageInfo->damage-= damageInfo->absorb + damageInfo->resist;
}

void Unit::CalculateHealAbsorb(const uint32 heal, uint32 *absorb)
{
    if (!isAlive() || !heal)
        return;

    int32 RemainingHeal = heal;

    // Need remove expired auras after
    bool existExpired = false;

    // absorb
    AuraList const& vHealAbsorb = GetAurasByType(SPELL_AURA_HEAL_ABSORB);
    for(AuraList::const_iterator i = vHealAbsorb.begin(); i != vHealAbsorb.end() && RemainingHeal > 0; ++i)
    {
        Modifier* mod = (*i)->GetModifier();

        // Max Amount can be absorbed by this aura
        int32  currentAbsorb = mod->m_amount;

        // Found empty aura (impossible but..)
        if (currentAbsorb <=0)
        {
            existExpired = true;
            continue;
        }

        // currentAbsorb - heal can be absorbed
        // If need absorb less heal
        if (RemainingHeal < currentAbsorb)
            currentAbsorb = RemainingHeal;

        RemainingHeal -= currentAbsorb;

        // Reduce aura amount
        mod->m_amount -= currentAbsorb;
        if ((*i)->GetHolder()->DropAuraCharge())
            mod->m_amount = 0;
        // Need remove it later
        if (mod->m_amount<=0)
            existExpired = true;
    }

    // Remove all expired absorb auras
    if (existExpired)
    {
        for(AuraList::const_iterator i = vHealAbsorb.begin(); i != vHealAbsorb.end();)
        {
            if ((*i)->GetModifier()->m_amount<=0)
            {
                RemoveAurasDueToSpell((*i)->GetId(), NULL, AURA_REMOVE_BY_SHIELD_BREAK);
                i = vHealAbsorb.begin();
            }
            else
                ++i;
        }
    }

    *absorb = heal - RemainingHeal;
}

void Unit::AttackerStateUpdate (Unit *pVictim, WeaponAttackType attType, bool extra )
{
    if(hasUnitState(UNIT_STAT_CAN_NOT_REACT) || HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED) )
        return;

    if (!pVictim->isAlive())
        return;

    if(IsNonMeleeSpellCasted(false))
        return;

    uint32 hitInfo;
    if (attType == BASE_ATTACK)
        hitInfo = HITINFO_NORMALSWING2;
    else if (attType == OFF_ATTACK)
        hitInfo = HITINFO_LEFTSWING;
    else
        return;                                             // ignore ranged case

    uint32 extraAttacks = m_extraAttacks;

    // melee attack spell casted at main hand attack only
    if (attType == BASE_ATTACK && m_currentSpells[CURRENT_MELEE_SPELL])
    {
        m_currentSpells[CURRENT_MELEE_SPELL]->cast();

        // not recent extra attack only at any non extra attack (melee spell case)
        if(!extra && extraAttacks)
        {
            while(m_extraAttacks)
            {
                AttackerStateUpdate(pVictim, BASE_ATTACK, true);
                if(m_extraAttacks > 0)
                    --m_extraAttacks;
            }
        }
        return;
    }

    // attack can be redirected to another target
    pVictim = SelectMagnetTarget(pVictim);

    CalcDamageInfo damageInfo;
    CalculateMeleeDamage(pVictim, 0, &damageInfo, attType);
    // Send log damage message to client
    DealDamageMods(pVictim,damageInfo.damage,&damageInfo.absorb);
    SendAttackStateUpdate(&damageInfo);
    ProcDamageAndSpell(damageInfo.target, damageInfo.procAttacker, damageInfo.procVictim, damageInfo.procEx, damageInfo.damage, damageInfo.attackType);
    DealMeleeDamage(&damageInfo,true);

    if (GetTypeId() == TYPEID_PLAYER)
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT,"AttackerStateUpdate: (Player) %u attacked %u (TypeId: %u) for %u dmg, absorbed %u, blocked %u, resisted %u.",
            GetGUIDLow(), pVictim->GetGUIDLow(), pVictim->GetTypeId(), damageInfo.damage, damageInfo.absorb, damageInfo.blocked_amount, damageInfo.resist);
    else
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT,"AttackerStateUpdate: (NPC)    %u attacked %u (TypeId: %u) for %u dmg, absorbed %u, blocked %u, resisted %u.",
            GetGUIDLow(), pVictim->GetGUIDLow(), pVictim->GetTypeId(), damageInfo.damage, damageInfo.absorb, damageInfo.blocked_amount, damageInfo.resist);

    // if damage pVictim call AI reaction
    if(pVictim->GetTypeId()==TYPEID_UNIT && ((Creature*)pVictim)->AI())
        ((Creature*)pVictim)->AI()->AttackedBy(this);

    // extra attack only at any non extra attack (normal case)
    if(!extra && extraAttacks)
    {
        while(m_extraAttacks)
        {
            AttackerStateUpdate(pVictim, BASE_ATTACK, true);
            if(m_extraAttacks > 0)
                --m_extraAttacks;
        }
    }
}

MeleeHitOutcome Unit::RollMeleeOutcomeAgainst(const Unit *pVictim, WeaponAttackType attType) const
{
    // This is only wrapper

    // Miss chance based on melee
    float miss_chance = MeleeMissChanceCalc(pVictim, attType);

    // Critical hit chance
    float crit_chance = GetUnitCriticalChance(attType, pVictim);

    // stunned target cannot dodge and this is check in GetUnitDodgeChance() (returned 0 in this case)
    float dodge_chance = pVictim->GetUnitDodgeChance();
    float block_chance = pVictim->GetUnitBlockChance();
    float parry_chance = pVictim->GetUnitParryChance();

    // Useful if want to specify crit & miss chances for melee, else it could be removed
    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT,"MELEE OUTCOME: miss %f crit %f dodge %f parry %f block %f", miss_chance,crit_chance,dodge_chance,parry_chance,block_chance);

    return RollMeleeOutcomeAgainst(pVictim, attType, int32(crit_chance*100), int32(miss_chance*100), int32(dodge_chance*100),int32(parry_chance*100),int32(block_chance*100));
}

MeleeHitOutcome Unit::RollMeleeOutcomeAgainst (const Unit *pVictim, WeaponAttackType attType, int32 crit_chance, int32 miss_chance, int32 dodge_chance, int32 parry_chance, int32 block_chance) const
{
    if(pVictim->GetTypeId()==TYPEID_UNIT && ((Creature*)pVictim)->IsInEvadeMode())
        return MELEE_HIT_EVADE;

    int32 attackerMaxSkillValueForLevel = GetMaxSkillValueForLevel(pVictim);
    int32 victimMaxSkillValueForLevel = pVictim->GetMaxSkillValueForLevel(this);

    int32 attackerWeaponSkill = GetWeaponSkillValue(attType,pVictim);
    int32 victimDefenseSkill = pVictim->GetDefenseSkillValue(this);

    // bonus from skills is 0.04%
    int32    skillBonus  = 4 * ( attackerWeaponSkill - victimMaxSkillValueForLevel );
    int32    sum = 0, tmp = 0;
    int32    roll = urand (0, 10000);

    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: skill bonus of %d for attacker", skillBonus);
    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: rolled %d, miss %d, dodge %d, parry %d, block %d, crit %d",
        roll, miss_chance, dodge_chance, parry_chance, block_chance, crit_chance);

    tmp = miss_chance;

    if (tmp > 0 && roll < (sum += tmp ))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: MISS");
        return MELEE_HIT_MISS;
    }

    // always crit against a sitting target (except 0 crit chance)
    if( pVictim->GetTypeId() == TYPEID_PLAYER && crit_chance > 0 && !pVictim->IsStandState() )
    {
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: CRIT (sitting victim)");
        return MELEE_HIT_CRIT;
    }

    bool from_behind = !pVictim->HasInArc(M_PI_F,this);

    if (from_behind)
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: attack came from behind.");

    // Dodge chance

    // only players can't dodge if attacker is behind
    if (pVictim->GetTypeId() != TYPEID_PLAYER || !from_behind)
    {
        // Reduce dodge chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            dodge_chance -= int32(((Player*)this)->GetExpertiseDodgeOrParryReduction(attType)*100);
        else
            dodge_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;

        // Modify dodge chance by attacker SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
        dodge_chance+= GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_COMBAT_RESULT_CHANCE, VICTIMSTATE_DODGE)*100;

        tmp = dodge_chance;
        if (   (tmp > 0)                                        // check if unit _can_ dodge
            && ((tmp -= skillBonus) > 0)
            && roll < (sum += tmp))
        {
            DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: DODGE <%d, %d)", sum-tmp, sum);
            return MELEE_HIT_DODGE;
        }
    }

    // parry chances
    // check if attack comes from behind, nobody can parry or block if attacker is behind
    if (!from_behind)
    {
        // Reduce parry chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            parry_chance-= int32(((Player*)this)->GetExpertiseDodgeOrParryReduction(attType)*100);
        else
            parry_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;

        if(pVictim->GetTypeId()==TYPEID_PLAYER || !(((Creature*)pVictim)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_PARRY) )
        {
            int32 tmp2 = int32(parry_chance);
            if (   (tmp2 > 0)                                   // check if unit _can_ parry
                && ((tmp2 -= skillBonus) > 0)
                && (roll < (sum += tmp2)))
            {
                DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: PARRY <%d, %d)", sum-tmp2, sum);
                return MELEE_HIT_PARRY;
            }
        }
    }

    // Max 40% chance to score a glancing blow against mobs that are higher level (can do only players and pets and not with ranged weapon)
    if( attType != RANGED_ATTACK &&
        (GetTypeId() == TYPEID_PLAYER || ((Creature*)this)->IsPet()) &&
        pVictim->GetTypeId() != TYPEID_PLAYER && !((Creature*)pVictim)->IsPet() &&
        getLevel() < pVictim->GetLevelForTarget(this) )
    {
        // cap possible value (with bonuses > max skill)
        int32 skill = attackerWeaponSkill;
        int32 maxskill = attackerMaxSkillValueForLevel;
        skill = (skill > maxskill) ? maxskill : skill;

        tmp = (10 + (victimDefenseSkill - skill)) * 100;
        tmp = tmp > 4000 ? 4000 : tmp;
        if (roll < (sum += tmp))
        {
            DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: GLANCING <%d, %d)", sum-4000, sum);
            return MELEE_HIT_GLANCING;
        }
    }

    // block chances
    // check if attack comes from behind, nobody can parry or block if attacker is behind
    if (!from_behind)
    {
        if(pVictim->GetTypeId()==TYPEID_PLAYER || !(((Creature*)pVictim)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_BLOCK) )
        {
            tmp = block_chance;
            if (   (tmp > 0)                                    // check if unit _can_ block
                && ((tmp -= skillBonus) > 0)
                && (roll < (sum += tmp)))
            {
                DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: BLOCK <%d, %d)", sum-tmp, sum);
                return MELEE_HIT_BLOCK;
            }
        }
    }

    // Critical chance
    tmp = crit_chance;

    if (tmp > 0 && roll < (sum += tmp))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: CRIT <%d, %d)", sum-tmp, sum);
        return MELEE_HIT_CRIT;
    }

    // mobs can score crushing blows if they're 4 or more levels above victim
    if (GetLevelForTarget(pVictim) >= pVictim->GetLevelForTarget(this) + 4 &&
        // can be from by creature (if can) or from controlled player that considered as creature
        ((GetTypeId()!=TYPEID_PLAYER && !((Creature*)this)->IsPet() &&
        !(((Creature*)this)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_CRUSH)) ||
        GetTypeId()==TYPEID_PLAYER && !GetCharmerOrOwnerGuid().IsEmpty()))
    {
        // when their weapon skill is 15 or more above victim's defense skill
        tmp = victimDefenseSkill;
        int32 tmpmax = victimMaxSkillValueForLevel;
        // having defense above your maximum (from items, talents etc.) has no effect
        tmp = tmp > tmpmax ? tmpmax : tmp;
        // tmp = mob's level * 5 - player's current defense skill
        tmp = attackerMaxSkillValueForLevel - tmp;
        if(tmp >= 15)
        {
            // add 2% chance per lacking skill point, min. is 15%
            tmp = tmp * 200 - 1500;
            if (roll < (sum += tmp))
            {
                DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: CRUSHING <%d, %d)", sum-tmp, sum);
                return MELEE_HIT_CRUSHING;
            }
        }
    }

    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "RollMeleeOutcomeAgainst: NORMAL");
    return MELEE_HIT_NORMAL;
}

uint32 Unit::CalculateDamage (WeaponAttackType attType, bool normalized)
{
    float min_damage, max_damage;

    if (normalized && GetTypeId()==TYPEID_PLAYER)
        ((Player*)this)->CalculateMinMaxDamage(attType,normalized,min_damage, max_damage);
    else
    {
        switch (attType)
        {
            case RANGED_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE);
                break;
            case BASE_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXDAMAGE);
                break;
            case OFF_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE);
                break;
                // Just for good manner
            default:
                min_damage = 0.0f;
                max_damage = 0.0f;
                break;
        }
    }

    if (min_damage > max_damage)
    {
        std::swap(min_damage,max_damage);
    }

    if(max_damage == 0.0f)
        max_damage = 5.0f;

    return urand((uint32)min_damage, (uint32)max_damage);
}

float Unit::CalculateLevelPenalty(SpellEntry const* spellProto) const
{
    if(!spellProto->spellLevel || !spellProto->maxLevel)
        return 1.0f;

    if(spellProto->maxLevel <= 0)
        return 1.0f;
    //if caster level is lower that max caster level
    if(getLevel() < spellProto->maxLevel)
        return 1.0f;

    float LvlPenalty = 0.0f;

    LvlPenalty = (22.0f + float (spellProto->maxLevel) - float (getLevel())) / 20.0f;
    //to prevent positive effect
    if(LvlPenalty > 1.0f)
        return 1.0f;
    //level penalty is capped at 0
    if(LvlPenalty < 0.0f)
        return 0.0f;

    return LvlPenalty;
}

void Unit::SendMeleeAttackStart(Unit* pVictim)
{
    WorldPacket data( SMSG_ATTACKSTART, 8 + 8 );
    data << GetObjectGuid();
    data << pVictim->GetObjectGuid();

    SendMessageToSet(&data, true);
    DEBUG_LOG( "WORLD: Sent SMSG_ATTACKSTART" );
}

void Unit::SendMeleeAttackStop(Unit* victim)
{
    if(!victim)
        return;

    WorldPacket data( SMSG_ATTACKSTOP, (4+16) );            // we guess size
    data << GetPackGUID();
    data << victim->GetPackGUID();                          // can be 0x00...
    data << uint32(0);                                      // can be 0x1
    SendMessageToSet(&data, true);
    DETAIL_FILTER_LOG(LOG_FILTER_COMBAT, "%s %u stopped attacking %s %u", (GetTypeId()==TYPEID_PLAYER ? "player" : "creature"), GetGUIDLow(), (victim->GetTypeId()==TYPEID_PLAYER ? "player" : "creature"),victim->GetGUIDLow());

    /*if(victim->GetTypeId() == TYPEID_UNIT)
    ((Creature*)victim)->AI().EnterEvadeMode(this);*/
}

bool Unit::IsSpellBlocked(Unit *pCaster, SpellEntry const *spellEntry, WeaponAttackType attackType)
{
    if (!HasInArc(M_PI_F, pCaster))
        return false;

    if (spellEntry)
    {
        // Some spells cannot be blocked
        if (spellEntry->Attributes & SPELL_ATTR_IMPOSSIBLE_DODGE_PARRY_BLOCK)
            return false;
    }

    /*
    // Ignore combat result aura (parry/dodge check on prepare)
    AuraList const& ignore = GetAurasByType(SPELL_AURA_IGNORE_COMBAT_RESULT);
    for(AuraList::const_iterator i = ignore.begin(); i != ignore.end(); ++i)
    {
        if (!(*i)->isAffectedOnSpell(spellProto))
            continue;
        if ((*i)->GetModifier()->m_miscvalue == ???)
            return false;
    }
    */

    // Check creatures flags_extra for disable block
    if (GetTypeId() == TYPEID_UNIT)
    {
        if (((Creature*)this)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_BLOCK)
            return false;
    }

    float blockChance = GetUnitBlockChance();
    blockChance += (int32(pCaster->GetWeaponSkillValue(attackType)) - int32(GetMaxSkillValueForLevel()))*0.04f;

    return roll_chance_f(blockChance);
}

// Melee based spells can be miss, parry or dodge on this step
// Crit or block - determined on damage calculation phase! (and can be both in some time)
float Unit::MeleeSpellMissChance(Unit *pVictim, WeaponAttackType attType, int32 skillDiff, SpellEntry const *spell)
{
    // Calculate hit chance (more correct for chance mod)
    float hitChance = 0.0f;

    // PvP - PvE melee chances
    // TODO: implement diminishing returns for defense from player's defense rating
    // pure skill diff is not sufficient since 3.x anymore, but exact formulas hard to research
    if (pVictim->GetTypeId() == TYPEID_PLAYER)
        hitChance = 95.0f + skillDiff * 0.04f;
    else if (skillDiff < -10)
        hitChance = 94.0f + (skillDiff + 10) * 0.4f;
    else
        hitChance = 95.0f + skillDiff * 0.1f;

    // Hit chance depends from victim auras
    if (attType == RANGED_ATTACK)
        hitChance += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE);
    else
        hitChance += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE);

    // Spellmod from SPELLMOD_RESIST_MISS_CHANCE
    if (Player *modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spell->Id, SPELLMOD_RESIST_MISS_CHANCE, hitChance);

    // Miss = 100 - hit
    float missChance = 100.0f - hitChance;

    // Bonuses from attacker aura and ratings
    if (attType == RANGED_ATTACK)
        missChance -= m_modRangedHitChance;
    else
        missChance -= m_modMeleeHitChance;

    // Limit miss chance from 0 to 60%
    if (missChance < 0.0f)
        return 0.0f;
    if (missChance > 60.0f)
        return 60.0f;
    return missChance;
}

// Melee based spells hit result calculations
SpellMissInfo Unit::MeleeSpellHitResult(Unit *pVictim, SpellEntry const *spell)
{
    WeaponAttackType attType = BASE_ATTACK;

    if (spell->DmgClass == SPELL_DAMAGE_CLASS_RANGED)
        attType = RANGED_ATTACK;

    // bonus from skills is 0.04% per skill Diff
    int32 attackerWeaponSkill = (spell->EquippedItemClass == ITEM_CLASS_WEAPON) ? int32(GetWeaponSkillValue(attType,pVictim)) : GetMaxSkillValueForLevel();
    int32 skillDiff = attackerWeaponSkill - int32(pVictim->GetMaxSkillValueForLevel(this));
    int32 fullSkillDiff = attackerWeaponSkill - int32(pVictim->GetDefenseSkillValue(this));

    uint32 roll = urand (0, 10000);

    uint32 missChance = uint32(MeleeSpellMissChance(pVictim, attType, fullSkillDiff, spell)*100.0f);
    // Roll miss
    uint32 tmp = missChance;
    if (roll < tmp)
        return SPELL_MISS_MISS;

    // Chance resist mechanic (select max value from every mechanic spell effect)
    int32 resist_mech = 0;
    // Get effects mechanic and chance
    for(int eff = 0; eff < MAX_EFFECT_INDEX; ++eff)
    {
        int32 effect_mech = GetEffectMechanic(spell, SpellEffectIndex(eff));
        if (effect_mech)
        {
            int32 temp = pVictim->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MECHANIC_RESISTANCE, effect_mech);
            if (resist_mech < temp*100)
                resist_mech = temp*100;
        }
    }
    // Roll chance
    tmp += resist_mech;
    if (roll < tmp)
        return SPELL_MISS_RESIST;

    bool canDodge = true;
    bool canParry = true;

    // Same spells cannot be parry/dodge
    if (spell->Attributes & SPELL_ATTR_IMPOSSIBLE_DODGE_PARRY_BLOCK)
        return SPELL_MISS_NONE;

    // Ranged attack cannot be parry/dodge only deflect
    if (attType == RANGED_ATTACK)
    {
        // only if in front
        if (pVictim->HasInArc(M_PI_F,this))
        {
            int32 deflect_chance = pVictim->GetTotalAuraModifier(SPELL_AURA_DEFLECT_SPELLS)*100;
            tmp+=deflect_chance;
            if (roll < tmp)
                return SPELL_MISS_DEFLECT;
        }
        return SPELL_MISS_NONE;
    }

    // Check for attack from behind
    if (!pVictim->HasInArc(M_PI_F,this))
    {
        // Can`t dodge from behind in PvP (but its possible in PvE)
        if (GetTypeId() == TYPEID_PLAYER && pVictim->GetTypeId() == TYPEID_PLAYER)
            canDodge = false;
        // Can`t parry
        canParry = false;
    }
    // Check creatures flags_extra for disable parry
    if(pVictim->GetTypeId()==TYPEID_UNIT)
    {
        uint32 flagEx = ((Creature*)pVictim)->GetCreatureInfo()->flags_extra;
        if( flagEx & CREATURE_FLAG_EXTRA_NO_PARRY )
            canParry = false;
    }
    // Ignore combat result aura
    AuraList const& ignore = GetAurasByType(SPELL_AURA_IGNORE_COMBAT_RESULT);
    for(AuraList::const_iterator i = ignore.begin(); i != ignore.end(); ++i)
    {
        if (!(*i)->isAffectedOnSpell(spell))
            continue;
        switch((*i)->GetModifier()->m_miscvalue)
        {
            case MELEE_HIT_DODGE: canDodge = false; break;
            case MELEE_HIT_BLOCK: break; // Block check in hit step
            case MELEE_HIT_PARRY: canParry = false; break;
            default:
                DEBUG_LOG("Spell %u SPELL_AURA_IGNORE_COMBAT_RESULT have unhandled state %d", (*i)->GetId(), (*i)->GetModifier()->m_miscvalue);
                break;
        }
    }

    if (canDodge)
    {
        // Roll dodge
        int32 dodgeChance = int32(pVictim->GetUnitDodgeChance()*100.0f) - skillDiff * 4;
        // Reduce enemy dodge chance by SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
        dodgeChance+= GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_COMBAT_RESULT_CHANCE, VICTIMSTATE_DODGE)*100;
        // Reduce dodge chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            dodgeChance-=int32(((Player*)this)->GetExpertiseDodgeOrParryReduction(attType) * 100.0f);
        else
            dodgeChance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;
        if (dodgeChance < 0)
            dodgeChance = 0;

        tmp += dodgeChance;
        if (roll < tmp)
            return SPELL_MISS_DODGE;
    }

    if (canParry)
    {
        // Roll parry
        int32 parryChance = int32(pVictim->GetUnitParryChance()*100.0f)  - skillDiff * 4;
        // Reduce parry chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            parryChance-=int32(((Player*)this)->GetExpertiseDodgeOrParryReduction(attType) * 100.0f);
        else
            parryChance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;
        if (parryChance < 0)
            parryChance = 0;

        tmp += parryChance;
        if (roll < tmp)
            return SPELL_MISS_PARRY;
    }

    return SPELL_MISS_NONE;
}

// TODO need use unit spell resistances in calculations
SpellMissInfo Unit::MagicSpellHitResult(Unit *pVictim, SpellEntry const *spell)
{
    // Can`t miss on dead target (on skinning for example)
    if (!pVictim->isAlive())
        return SPELL_MISS_NONE;

    SpellSchoolMask schoolMask = GetSpellSchoolMask(spell);
    // PvP - PvE spell misschances per leveldif > 2
    int32 lchance = pVictim->GetTypeId() == TYPEID_PLAYER ? 7 : 11;
    int32 leveldif = int32(pVictim->GetLevelForTarget(this)) - int32(GetLevelForTarget(pVictim));

    // Base hit chance from attacker and victim levels
    int32 modHitChance;
    if(leveldif < 3)
        modHitChance = 96 - leveldif;
    else
        modHitChance = 94 - (leveldif - 2) * lchance;

    // Spellmod from SPELLMOD_RESIST_MISS_CHANCE
    if(Player *modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spell->Id, SPELLMOD_RESIST_MISS_CHANCE, modHitChance);
    // Increase from attacker SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT auras
    modHitChance+=GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT, schoolMask);
    // Chance hit from victim SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE auras
    modHitChance+= pVictim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE, schoolMask);
    // Reduce spell hit chance for Area of effect spells from victim SPELL_AURA_MOD_AOE_AVOIDANCE aura
    if (IsAreaOfEffectSpell(spell))
        modHitChance-=pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_AOE_AVOIDANCE);
    // Reduce spell hit chance for dispel mechanic spells from victim SPELL_AURA_MOD_DISPEL_RESIST
    if (IsDispelSpell(spell))
        modHitChance-=pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_DISPEL_RESIST);
    // Chance resist mechanic (select max value from every mechanic spell effect)
    int32 resist_mech = 0;
    // Get effects mechanic and chance
    for(int eff = 0; eff < MAX_EFFECT_INDEX; ++eff)
    {
        int32 effect_mech = GetEffectMechanic(spell, SpellEffectIndex(eff));
        if (effect_mech)
        {
            int32 temp = pVictim->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MECHANIC_RESISTANCE, effect_mech);
            if (resist_mech < temp)
                resist_mech = temp;
        }
    }
    // Apply mod
    modHitChance-=resist_mech;

    // Chance resist debuff
    modHitChance-=pVictim->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DEBUFF_RESISTANCE, int32(spell->Dispel));

    int32 HitChance = modHitChance * 100;
    // Increase hit chance from attacker SPELL_AURA_MOD_SPELL_HIT_CHANCE and attacker ratings
    HitChance += int32(m_modSpellHitChance*100.0f);

    // Decrease hit chance from victim rating bonus
    if (pVictim->GetTypeId()==TYPEID_PLAYER)
        HitChance -= int32(((Player*)pVictim)->GetRatingBonusValue(CR_HIT_TAKEN_SPELL)*100.0f);

    if (HitChance <  100) HitChance =  100;
    if (HitChance > 10000) HitChance = 10000;

    int32 tmp = 10000 - HitChance;

    int32 rand = irand(0,10000);

    if (rand < tmp)
        return SPELL_MISS_MISS;

    // cast by caster in front of victim
    if (pVictim->HasInArc(M_PI_F,this))
    {
        int32 deflect_chance = pVictim->GetTotalAuraModifier(SPELL_AURA_DEFLECT_SPELLS)*100;
        tmp+=deflect_chance;
        if (rand < tmp)
            return SPELL_MISS_DEFLECT;
    }

    return SPELL_MISS_NONE;
}

// Calculate spell hit result can be:
// Every spell can: Evade/Immune/Reflect/Sucesful hit
// For melee based spells:
//   Miss
//   Dodge
//   Parry
// For spells
//   Resist
SpellMissInfo Unit::SpellHitResult(Unit *pVictim, SpellEntry const *spell, bool CanReflect)
{
    // Return evade for units in evade mode
    if (pVictim->GetTypeId()==TYPEID_UNIT && ((Creature*)pVictim)->IsInEvadeMode())
        return SPELL_MISS_EVADE;

    if (!(spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY))
    {
        // Check for immune
        if (pVictim->IsImmuneToSpell(spell))
            return SPELL_MISS_IMMUNE;

        // All positive spells can`t miss
        // TODO: client not show miss log for this spells - so need find info for this in dbc and use it!
        if (IsPositiveSpell(spell->Id) && IsFriendlyTo(pVictim))
            return SPELL_MISS_NONE;

        // Check for immune
        if (pVictim->IsImmunedToDamage(GetSpellSchoolMask(spell)))
            return SPELL_MISS_IMMUNE;
    }
    else if (IsPositiveSpell(spell->Id) && IsFriendlyTo(pVictim))
        return SPELL_MISS_NONE;

    // Try victim reflect spell
    if (CanReflect)
    {
        int32 reflectchance = pVictim->GetTotalAuraModifier(SPELL_AURA_REFLECT_SPELLS);
        Unit::AuraList const& mReflectSpellsSchool = pVictim->GetAurasByType(SPELL_AURA_REFLECT_SPELLS_SCHOOL);
        for(Unit::AuraList::const_iterator i = mReflectSpellsSchool.begin(); i != mReflectSpellsSchool.end(); ++i)
            if((*i)->GetModifier()->m_miscvalue & GetSpellSchoolMask(spell))
                reflectchance += (*i)->GetModifier()->m_amount;
        if (reflectchance > 0 && roll_chance_i(reflectchance))
        {
            // Start triggers for remove charges if need (trigger only for victim, and mark as active spell)
            ProcDamageAndSpell(pVictim, PROC_FLAG_NONE, PROC_FLAG_TAKEN_NEGATIVE_SPELL_HIT, PROC_EX_REFLECT, 1, BASE_ATTACK, spell);
            return SPELL_MISS_REFLECT;
        }
    }

    switch (spell->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_NONE:
            return SPELL_MISS_NONE;
        case SPELL_DAMAGE_CLASS_MAGIC:
            return MagicSpellHitResult(pVictim, spell);
        case SPELL_DAMAGE_CLASS_MELEE:
        case SPELL_DAMAGE_CLASS_RANGED:
            return MeleeSpellHitResult(pVictim, spell);
    }
    return SPELL_MISS_NONE;
}

float Unit::MeleeMissChanceCalc(const Unit *pVictim, WeaponAttackType attType) const
{
    if(!pVictim)
        return 0.0f;

    // Base misschance 5%
    float missChance = 5.0f;

    // DualWield - white damage has additional 19% miss penalty
    if (haveOffhandWeapon() && attType != RANGED_ATTACK)
    {
        bool isNormal = false;
        for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
        {
            if (m_currentSpells[i] && (GetSpellSchoolMask(m_currentSpells[i]->m_spellInfo) & SPELL_SCHOOL_MASK_NORMAL))
            {
                isNormal = true;
                break;
            }
        }
        if (!isNormal && !m_currentSpells[CURRENT_MELEE_SPELL])
            missChance += 19.0f;
    }

    int32 skillDiff = int32(GetWeaponSkillValue(attType, pVictim)) - int32(pVictim->GetDefenseSkillValue(this));

    // PvP - PvE melee chances
    // TODO: implement diminishing returns for defense from player's defense rating
    // pure skill diff is not sufficient since 3.x anymore, but exact formulas hard to research
    if ( pVictim->GetTypeId() == TYPEID_PLAYER )
        missChance -= skillDiff * 0.04f;
    else if ( skillDiff < -10 )
        missChance -= (skillDiff + 10) * 0.4f - 1.0f;
    else
        missChance -=  skillDiff * 0.1f;

    // Hit chance bonus from attacker based on ratings and auras
    if (attType == RANGED_ATTACK)
        missChance -= m_modRangedHitChance;
    else
        missChance -= m_modMeleeHitChance;

    // Hit chance for victim based on ratings
    if (pVictim->GetTypeId()==TYPEID_PLAYER)
    {
        if (attType == RANGED_ATTACK)
            missChance += ((Player*)pVictim)->GetRatingBonusValue(CR_HIT_TAKEN_RANGED);
        else
            missChance += ((Player*)pVictim)->GetRatingBonusValue(CR_HIT_TAKEN_MELEE);
    }

    // Modify miss chance by victim auras
    if(attType == RANGED_ATTACK)
        missChance -= pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE);
    else
        missChance -= pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE);

    // Limit miss chance from 0 to 60%
    if (missChance < 0.0f)
        return 0.0f;
    if (missChance > 60.0f)
        return 60.0f;

    return missChance;
}

uint32 Unit::GetDefenseSkillValue(Unit const* target) const
{
    if(GetTypeId() == TYPEID_PLAYER)
    {
        // in PvP use full skill instead current skill value
        uint32 value = (target && target->GetTypeId() == TYPEID_PLAYER)
            ? ((Player*)this)->GetMaxSkillValue(SKILL_DEFENSE)
            : ((Player*)this)->GetSkillValue(SKILL_DEFENSE);
        value += uint32(((Player*)this)->GetRatingBonusValue(CR_DEFENSE_SKILL));
        return value;
    }
    else
        return GetUnitMeleeSkill(target);
}

float Unit::GetUnitDodgeChance() const
{
    if(hasUnitState(UNIT_STAT_STUNNED))
        return 0.0f;
    if( GetTypeId() == TYPEID_PLAYER )
        return GetFloatValue(PLAYER_DODGE_PERCENTAGE);
    else
    {
        if(((Creature const*)this)->IsTotem())
            return 0.0f;
        else
        {
            float dodge = 5.0f;
            dodge += GetTotalAuraModifier(SPELL_AURA_MOD_DODGE_PERCENT);
            return dodge > 0.0f ? dodge : 0.0f;
        }
    }
}

float Unit::GetUnitParryChance() const
{
    if ( IsNonMeleeSpellCasted(false) || hasUnitState(UNIT_STAT_STUNNED))
        return 0.0f;

    float chance = 0.0f;

    if(GetTypeId() == TYPEID_PLAYER)
    {
        Player const* player = (Player const*)this;
        if(player->CanParry() )
        {
            Item *tmpitem = player->GetWeaponForAttack(BASE_ATTACK,true,true);
            if(!tmpitem)
                tmpitem = player->GetWeaponForAttack(OFF_ATTACK,true,true);

            if(tmpitem)
                chance = GetFloatValue(PLAYER_PARRY_PERCENTAGE);
        }
    }
    else if(GetTypeId() == TYPEID_UNIT)
    {
        if(GetCreatureType() == CREATURE_TYPE_HUMANOID)
        {
            chance = 5.0f;
            chance += GetTotalAuraModifier(SPELL_AURA_MOD_PARRY_PERCENT);
        }
    }

    return chance > 0.0f ? chance : 0.0f;
}

float Unit::GetUnitBlockChance() const
{
    if ( IsNonMeleeSpellCasted(false) || hasUnitState(UNIT_STAT_STUNNED))
        return 0.0f;

    if(GetTypeId() == TYPEID_PLAYER)
    {
        Player const* player = (Player const*)this;
        if(player->CanBlock() && player->CanUseEquippedWeapon(OFF_ATTACK))
        {
            Item *tmpitem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if(tmpitem && !tmpitem->IsBroken() && tmpitem->GetProto()->Block)
                return GetFloatValue(PLAYER_BLOCK_PERCENTAGE);
        }
        // is player but has no block ability or no not broken shield equipped
        return 0.0f;
    }
    else
    {
        if(((Creature const*)this)->IsTotem())
            return 0.0f;
        else
        {
            float block = 5.0f;
            block += GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_PERCENT);
            return block > 0.0f ? block : 0.0f;
        }
    }
}

float Unit::GetUnitCriticalChance(WeaponAttackType attackType, const Unit *pVictim) const
{
    float crit;

    if(GetTypeId() == TYPEID_PLAYER)
    {
        switch(attackType)
        {
            case BASE_ATTACK:
                crit = GetFloatValue( PLAYER_CRIT_PERCENTAGE );
                break;
            case OFF_ATTACK:
                crit = GetFloatValue( PLAYER_OFFHAND_CRIT_PERCENTAGE );
                break;
            case RANGED_ATTACK:
                crit = GetFloatValue( PLAYER_RANGED_CRIT_PERCENTAGE );
                break;
                // Just for good manner
            default:
                crit = 0.0f;
                break;
        }
    }
    else
    {
        crit = 5.0f;
        crit += GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PERCENT);
    }

    // flat aura mods
    if(attackType == RANGED_ATTACK)
        crit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE);
    else
        crit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE);

    crit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);

    // reduce crit chance from Rating for players
    if (attackType != RANGED_ATTACK)
        crit -= pVictim->GetMeleeCritChanceReduction();
    else
        crit -= pVictim->GetRangedCritChanceReduction();

    // Apply crit chance from defence skill
    crit += (int32(GetMaxSkillValueForLevel(pVictim)) - int32(pVictim->GetDefenseSkillValue(this))) * 0.04f;

    if (crit < 0.0f)
        crit = 0.0f;
    return crit;
}

uint32 Unit::GetWeaponSkillValue (WeaponAttackType attType, Unit const* target) const
{
    uint32 value = 0;
    if(GetTypeId() == TYPEID_PLAYER)
    {
        Item* item = ((Player*)this)->GetWeaponForAttack(attType,true,true);

        // feral or unarmed skill only for base attack
        if(attType != BASE_ATTACK && !item )
            return 0;

        if(IsInFeralForm())
            return GetMaxSkillValueForLevel();              // always maximized SKILL_FERAL_COMBAT in fact

        // weapon skill or (unarmed for base attack)
        uint32  skill = item ? item->GetSkill() : SKILL_UNARMED;

        // in PvP use full skill instead current skill value
        value = (target && target->GetTypeId() == TYPEID_PLAYER)
            ? ((Player*)this)->GetMaxSkillValue(skill)
            : ((Player*)this)->GetSkillValue(skill);
        // Modify value from ratings
        value += uint32(((Player*)this)->GetRatingBonusValue(CR_WEAPON_SKILL));
        switch (attType)
        {
            case BASE_ATTACK:   value+=uint32(((Player*)this)->GetRatingBonusValue(CR_WEAPON_SKILL_MAINHAND));break;
            case OFF_ATTACK:    value+=uint32(((Player*)this)->GetRatingBonusValue(CR_WEAPON_SKILL_OFFHAND));break;
            case RANGED_ATTACK: value+=uint32(((Player*)this)->GetRatingBonusValue(CR_WEAPON_SKILL_RANGED));break;
        }
    }
    else
        value = GetUnitMeleeSkill(target);
   return value;
}

void Unit::_UpdateSpells( uint32 time )
{
    if(m_currentSpells[CURRENT_AUTOREPEAT_SPELL])
        _UpdateAutoRepeatSpell();

    // remove finished spells from current pointers
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
    {
        if (m_currentSpells[i] && m_currentSpells[i]->getState() == SPELL_STATE_FINISHED)
        {
            m_currentSpells[i]->SetReferencedFromCurrent(false);
            m_currentSpells[i] = NULL;                      // remove pointer
        }
    }

    // update auras
    // m_AurasUpdateIterator can be updated in inderect called code at aura remove to skip next planned to update but removed auras
    for (m_spellAuraHoldersUpdateIterator = m_spellAuraHolders.begin(); m_spellAuraHoldersUpdateIterator != m_spellAuraHolders.end();)
    {
        SpellAuraHolder* i_holder = m_spellAuraHoldersUpdateIterator->second;
        ++m_spellAuraHoldersUpdateIterator;                            // need shift to next for allow update if need into aura update
        if (i_holder && !i_holder->IsDeleted() && !i_holder->IsEmptyHolder() && !i_holder->IsInUse())
            i_holder->UpdateHolder(time);
    }

    // remove expired auras
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        SpellAuraHolder *holder = iter->second;
        if (holder)
        {
            if (!(holder->IsPermanent() || holder->IsPassive()) )
            {
                bool removedAura = false;
                for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                {
                    if (Aura *aura = holder->GetAuraByEffectIndex(SpellEffectIndex(i)))
                    {
                        if (!aura->GetAuraDuration())
                        {
                            RemoveSingleAuraFromSpellAuraHolder(holder, aura->GetEffIndex(), AURA_REMOVE_BY_EXPIRE);
                            removedAura = true;
                            break;
                        }
                    }
                }

                if (!removedAura)
                    ++iter;
                else
                    iter = m_spellAuraHolders.begin();
            }
            else
                ++iter;
        }
        else
            ++iter;
    }

    if(!m_gameObj.empty())
    {
        GameObjectList::iterator ite1, dnext1;
        for (ite1 = m_gameObj.begin(); ite1 != m_gameObj.end(); ite1 = dnext1)
        {
            dnext1 = ite1;
            //(*i)->Update( difftime );
            if( !(*ite1)->isSpawned() )
            {
                (*ite1)->SetOwnerGuid(ObjectGuid());
                (*ite1)->SetRespawnTime(0);
                (*ite1)->Delete();
                dnext1 = m_gameObj.erase(ite1);
            }
            else
                ++dnext1;
        }
    }
}

void Unit::_UpdateAutoRepeatSpell()
{
    bool isAutoShot = m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id == SPELL_ID_AUTOSHOT;

    //check movement
    if (GetTypeId() == TYPEID_PLAYER && ((Player*)this)->isMoving())
    {
        // cancel wand shoot
        if(!isAutoShot)
            InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
        // auto shot just waits
        return;
    }

    // check spell casts
    if (IsNonMeleeSpellCasted(false, false, true))
    {
        // cancel wand shoot
        if(!isAutoShot)
        {
            InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            return;
        }
        // auto shot is delayed by everythihng, except ranged(!) CURRENT_GENERIC_SPELL's -> recheck that
        else if (!(m_currentSpells[CURRENT_GENERIC_SPELL] && m_currentSpells[CURRENT_GENERIC_SPELL]->IsRangedSpell()))
            return;
    }

    //castroutine
    if (isAttackReady(RANGED_ATTACK))
    {
        // Check if able to cast
        if(m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->CheckCast(true) != SPELL_CAST_OK)
        {
            InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            return;
        }

        // we want to shoot
        Spell* spell = new Spell(this, m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo, true);
        spell->prepare(&(m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_targets));

        // all went good, reset attack
        resetAttackTimer(RANGED_ATTACK);
    }
}

void Unit::SetCurrentCastedSpell( Spell * pSpell )
{
    MANGOS_ASSERT(pSpell);                                  // NULL may be never passed here, use InterruptSpell or InterruptNonMeleeSpells

    CurrentSpellTypes CSpellType = pSpell->GetCurrentContainer();

    if (pSpell == m_currentSpells[CSpellType]) return;      // avoid breaking self

    // break same type spell if it is not delayed
    InterruptSpell(CSpellType,false);

    // special breakage effects:
    switch (CSpellType)
    {
        case CURRENT_GENERIC_SPELL:
        {
            // generic spells always break channeled not delayed spells
            InterruptSpell(CURRENT_CHANNELED_SPELL,false);

            // autorepeat breaking
            if ( m_currentSpells[CURRENT_AUTOREPEAT_SPELL] )
            {
                // break autorepeat if not Auto Shot
                if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != SPELL_ID_AUTOSHOT)
                    InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            }
        } break;

        case CURRENT_CHANNELED_SPELL:
        {
            // channel spells always break generic non-delayed and any channeled spells
            InterruptSpell(CURRENT_GENERIC_SPELL,false);
            InterruptSpell(CURRENT_CHANNELED_SPELL);

            // it also does break autorepeat if not Auto Shot
            if ( m_currentSpells[CURRENT_AUTOREPEAT_SPELL] &&
                m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != SPELL_ID_AUTOSHOT )
                InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
        } break;

        case CURRENT_AUTOREPEAT_SPELL:
        {
            // only Auto Shoot does not break anything
            if (pSpell->m_spellInfo->Id != SPELL_ID_AUTOSHOT)
            {
                // generic autorepeats break generic non-delayed and channeled non-delayed spells
                InterruptSpell(CURRENT_GENERIC_SPELL,false);
                InterruptSpell(CURRENT_CHANNELED_SPELL,false);
                // special action: first cast delay
                if ( getAttackTimer(RANGED_ATTACK) < 500 )
                    setAttackTimer(RANGED_ATTACK,500);
            }
        } break;

        default:
        {
            // other spell types don't break anything now
        } break;
    }

    // current spell (if it is still here) may be safely deleted now
    if (m_currentSpells[CSpellType])
        m_currentSpells[CSpellType]->SetReferencedFromCurrent(false);

    // set new current spell
    m_currentSpells[CSpellType] = pSpell;
    pSpell->SetReferencedFromCurrent(true);

    pSpell->m_selfContainer = &(m_currentSpells[pSpell->GetCurrentContainer()]);
}

void Unit::InterruptSpell(CurrentSpellTypes spellType, bool withDelayed, bool sendAutoRepeatCancelToClient)
{
    MANGOS_ASSERT(spellType < CURRENT_MAX_SPELL);

    if (m_currentSpells[spellType] && (withDelayed || m_currentSpells[spellType]->getState() != SPELL_STATE_DELAYED) )
    {
        // send autorepeat cancel message for autorepeat spells
        if (spellType == CURRENT_AUTOREPEAT_SPELL && sendAutoRepeatCancelToClient)
        {
            if(GetTypeId() == TYPEID_PLAYER)
                ((Player*)this)->SendAutoRepeatCancel(this);
        }

        if (m_currentSpells[spellType]->getState() != SPELL_STATE_FINISHED)
            m_currentSpells[spellType]->cancel();

        // cancel can interrupt spell already (caster cancel ->target aura remove -> caster iterrupt)
        if (m_currentSpells[spellType])
        {
            m_currentSpells[spellType]->SetReferencedFromCurrent(false);
            m_currentSpells[spellType] = NULL;
        }
    }
}

void Unit::FinishSpell(CurrentSpellTypes spellType, bool ok /*= true*/)
{
    Spell* spell = m_currentSpells[spellType];
    if (!spell)
        return;

    if (spellType == CURRENT_CHANNELED_SPELL)
        spell->SendChannelUpdate(0);

    spell->finish(ok);
}


bool Unit::IsNonMeleeSpellCasted(bool withDelayed, bool skipChanneled, bool skipAutorepeat) const
{
    // We don't do loop here to explicitly show that melee spell is excluded.
    // Maybe later some special spells will be excluded too.

    // generic spells are casted when they are not finished and not delayed
    if ( m_currentSpells[CURRENT_GENERIC_SPELL] &&
        (m_currentSpells[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_FINISHED) &&
        (withDelayed || m_currentSpells[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_DELAYED) )
        {
            if (!(m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->AttributesEx2 & SPELL_ATTR_EX2_NOT_RESET_AUTOSHOT))
                return(true);
        }

    // channeled spells may be delayed, but they are still considered casted
    else if ( !skipChanneled && m_currentSpells[CURRENT_CHANNELED_SPELL] &&
        (m_currentSpells[CURRENT_CHANNELED_SPELL]->getState() != SPELL_STATE_FINISHED) )
        return(true);

    // autorepeat spells may be finished or delayed, but they are still considered casted
    else if ( !skipAutorepeat && m_currentSpells[CURRENT_AUTOREPEAT_SPELL] )
        return(true);

    return(false);
}

void Unit::InterruptNonMeleeSpells(bool withDelayed, uint32 spell_id)
{
    // generic spells are interrupted if they are not finished or delayed
    if (m_currentSpells[CURRENT_GENERIC_SPELL] && (!spell_id || m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Id==spell_id))
        InterruptSpell(CURRENT_GENERIC_SPELL,withDelayed);

    // autorepeat spells are interrupted if they are not finished or delayed
    if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL] && (!spell_id || m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id==spell_id))
        InterruptSpell(CURRENT_AUTOREPEAT_SPELL,withDelayed);

    // channeled spells are interrupted if they are not finished, even if they are delayed
    if (m_currentSpells[CURRENT_CHANNELED_SPELL] && (!spell_id || m_currentSpells[CURRENT_CHANNELED_SPELL]->m_spellInfo->Id==spell_id))
        InterruptSpell(CURRENT_CHANNELED_SPELL,true);
}

Spell* Unit::FindCurrentSpellBySpellId(uint32 spell_id) const
{
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
        if(m_currentSpells[i] && m_currentSpells[i]->m_spellInfo->Id==spell_id)
            return m_currentSpells[i];
    return NULL;
}

void Unit::SetInFront(Unit const* target)
{
    SetOrientation(GetAngle(target));
}

void Unit::SetFacingTo(float ori, bool bToSelf /*= false*/)
{
    // update orientation at server
    SetOrientation(ori);

    // and client
    SendHeartBeat(bToSelf);
}

// Consider move this to Creature:: since only creature appear to be able to use this
void Unit::SetFacingToObject(WorldObject* pObject)
{
    if (GetTypeId() != TYPEID_UNIT)
        return;

    // never face when already moving
    if (!IsStopped())
        return;

    // TODO: figure out under what conditions creature will move towards object instead of facing it where it currently is.

    SetOrientation(GetAngle(pObject));
    SendMonsterMove(GetPositionX(), GetPositionY(), GetPositionZ(), SPLINETYPE_FACINGTARGET, ((Creature*)this)->GetSplineFlags(), 0, NULL, pObject->GetGUID());
}

bool Unit::isInAccessablePlaceFor(Creature const* c) const
{
    if(IsInWater())
        return c->CanSwim();
    else
        return c->CanWalk() || c->CanFly();
}

bool Unit::IsInWater() const
{
    return GetTerrain()->IsInWater(GetPositionX(),GetPositionY(), GetPositionZ());
}

bool Unit::IsUnderWater() const
{
    return GetTerrain()->IsUnderWater(GetPositionX(),GetPositionY(),GetPositionZ());
}

void Unit::DeMorph()
{
    SetDisplayId(GetNativeDisplayId());
}

int32 Unit::GetTotalAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
        modifier += (*i)->GetModifier()->m_amount;

    return modifier;
}

float Unit::GetTotalAuraMultiplier(AuraType auratype) const
{
    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
        multiplier *= (100.0f + (*i)->GetModifier()->m_amount)/100.0f;

    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
        if ((*i)->GetModifier()->m_amount > modifier)
            modifier = (*i)->GetModifier()->m_amount;

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
        if ((*i)->GetModifier()->m_amount < modifier)
            modifier = (*i)->GetModifier()->m_amount;

    return modifier;
}

int32 Unit::GetTotalAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if(!misc_mask)
        return 0;

    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask)
            modifier += mod->m_amount;
    }
    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if(!misc_mask)
        return 1.0f;

    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask)
            multiplier *= (100.0f + mod->m_amount)/100.0f;
    }
    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if(!misc_mask)
        return 0;

    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask && mod->m_amount > modifier)
            modifier = mod->m_amount;
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if(!misc_mask)
        return 0;

    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask && mod->m_amount < modifier)
            modifier = mod->m_amount;
    }

    return modifier;
}

int32 Unit::GetTotalAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value)
            modifier += mod->m_amount;
    }
    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscValue(AuraType auratype, int32 misc_value) const
{
    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value)
            multiplier *= (100.0f + mod->m_amount)/100.0f;
    }
    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value && mod->m_amount > modifier)
            modifier = mod->m_amount;
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value && mod->m_amount < modifier)
            modifier = mod->m_amount;
    }

    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscValueForMask(AuraType auratype, uint32 mask) const
{
    if(!mask)
        return 1.0f;

    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for(AuraList::const_iterator i = mTotalAuraList.begin();i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mask & (1 << (mod->m_miscvalue -1)))
            multiplier *= (100.0f + mod->m_amount)/100.0f;
    }
    return multiplier;
}

bool Unit::AddSpellAuraHolder(SpellAuraHolder *holder)
{
    SpellEntry const* aurSpellInfo = holder->GetSpellProto();

    // ghost spell check, allow apply any auras at player loading in ghost mode (will be cleanup after load)
    if( !isAlive() && !IsDeathPersistentSpell(aurSpellInfo) &&
        !IsDeathOnlySpell(aurSpellInfo) &&
        (GetTypeId()!=TYPEID_PLAYER || !((Player*)this)->GetSession()->PlayerLoading()) )
    {
        delete holder;
        return false;
    }

    if(holder->GetTarget() != this)
    {
        sLog.outError("Holder (spell %u) add to spell aura holder list of %s (lowguid: %u) but spell aura holder target is %s (lowguid: %u)",
            holder->GetId(),(GetTypeId()==TYPEID_PLAYER?"player":"creature"),GetGUIDLow(),
            (holder->GetTarget()->GetTypeId()==TYPEID_PLAYER?"player":"creature"),holder->GetTarget()->GetGUIDLow());
        delete holder;
        return false;
    }

    // passive and persistent auras can stack with themselves any number of times
    if ((!holder->IsPassive() && !holder->IsPersistent()) || holder->IsAreaAura())
    {
        SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(aurSpellInfo->Id);

        // take out same spell
        for (SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second; ++iter)
        {
            SpellAuraHolder *foundHolder = iter->second;
            if(foundHolder->GetCasterGUID() == holder->GetCasterGUID())
            {
                // Aura can stack on self -> Stack it;
                if(aurSpellInfo->StackAmount)
                {
                    // can be created with >1 stack by some spell mods
                    foundHolder->ModStackAmount(holder->GetStackAmount());
                    delete holder;
                    return false;
                }

                // Check for coexisting Weapon-proced Auras
                if (holder->IsWeaponBuffCoexistableWith(foundHolder))
                    continue;

                // Carry over removed Aura's remaining damage if Aura still has ticks remaining
                if (foundHolder->GetSpellProto()->AttributesEx4 & SPELL_ATTR_EX4_STACK_DOT_MODIFIER)
                {
                    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                    {
                        if (Aura *aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i)))
                        {
                            // m_auraname can be modified to SPELL_AURA_NONE for area auras, use original
                            AuraType aurNameReal = AuraType(aurSpellInfo->EffectApplyAuraName[i]);

                            if (aurNameReal == SPELL_AURA_PERIODIC_DAMAGE && aur->GetAuraDuration() > 0)
                            {
                                if (Aura *existing = foundHolder->GetAuraByEffectIndex(SpellEffectIndex(i)))
                                {
                                    int32 remainingTicks = existing->GetAuraMaxTicks() - existing->GetAuraTicks();
                                    int32 remainingDamage = existing->GetModifier()->m_amount * remainingTicks;

                                    aur->GetModifier()->m_amount += int32(remainingDamage / aur->GetAuraMaxTicks());
                                }
                                else
                                    DEBUG_LOG("Holder (spell %u) on target (lowguid: %u) doesn't have aura on effect index %u. skipping.", aurSpellInfo->Id, holder->GetTarget()->GetGUIDLow(), i);
                            }
                        }
                    }
                }

                // can be only single
                RemoveSpellAuraHolder(foundHolder, AURA_REMOVE_BY_STACK);
                break;
            }

            bool stop = false;

            for (int32 i = 0; i < MAX_EFFECT_INDEX && !stop; ++i)
            {
                // no need to check non stacking auras that weren't/won't be applied on this target
                if (!foundHolder->m_auras[i] || !holder->m_auras[i])
                    continue;

                // m_auraname can be modified to SPELL_AURA_NONE for area auras, use original
                AuraType aurNameReal = AuraType(aurSpellInfo->EffectApplyAuraName[i]);

                switch(aurNameReal)
                {
                    // DoT/HoT/etc
                    case SPELL_AURA_DUMMY:                  // allow stack
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                    case SPELL_AURA_PERIODIC_LEECH:
                    case SPELL_AURA_PERIODIC_HEAL:
                    case SPELL_AURA_OBS_MOD_HEALTH:
                    case SPELL_AURA_PERIODIC_MANA_LEECH:
                    case SPELL_AURA_OBS_MOD_MANA:
                    case SPELL_AURA_POWER_BURN_MANA:
                        break;
                    case SPELL_AURA_PERIODIC_ENERGIZE:      // all or self or clear non-stackable
                    default:                                // not allow
                        // can be only single (this check done at _each_ aura add
                        RemoveSpellAuraHolder(foundHolder,AURA_REMOVE_BY_STACK);
                        stop = true;
                        break;
                }
            }

            if(stop)
                break;

        }
    }

    // passive auras not stackable with other ranks
    if (!IsPassiveSpellStackableWithRanks(aurSpellInfo))
    {
        if (!RemoveNoStackAurasDueToAuraHolder(holder))
        {
            delete holder;
            return false;                                   // couldn't remove conflicting aura with higher rank
        }
    }

    // update single target auras list (before aura add to aura list, to prevent unexpected remove recently added aura)
    if (holder->IsSingleTarget())
    {
        if (Unit* caster = holder->GetCaster())             // caster not in world
        {
            SingleCastSpellTargetMap& scTargets = caster->GetSingleCastSpellTargets();
            for(SingleCastSpellTargetMap::iterator itr = scTargets.begin(); itr != scTargets.end();)
            {
                SpellEntry const* itr_spellEntry = itr->first;
                ObjectGuid itr_targetGuid = itr->second;

                if (itr_targetGuid != GetObjectGuid() &&
                    IsSingleTargetSpells(itr_spellEntry, aurSpellInfo))
                {
                    scTargets.erase(itr);                   // remove for caster in any case

                    // remove from target if target found
                    if (Unit* itr_target = GetMap()->GetUnit(itr_targetGuid))
                        itr_target->RemoveAurasDueToSpell(itr_spellEntry->Id);

                    itr = scTargets.begin();                // list can be chnaged at remove aura
                    continue;
                }

                ++itr;
            }

            // register spell holder single target
            scTargets[aurSpellInfo] = GetObjectGuid();
        }
    }

    // add aura, register in lists and arrays
    holder->_AddSpellAuraHolder();
    m_spellAuraHolders.insert(SpellAuraHolderMap::value_type(holder->GetId(), holder));

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i)))
            AddAuraToModList(aur);

    holder->ApplyAuraModifiers(true, true);
    DEBUG_LOG("Holder of spell %u now is in use", holder->GetId());

    // if aura deleted before boosts apply ignore
    // this can be possible it it removed indirectly by triggered spell effect at ApplyModifier
    if (holder->IsDeleted())
        return false;

    holder->HandleSpellSpecificBoosts(true);

    return true;
}

void Unit::AddAuraToModList(Aura *aura)
{
    if (aura->GetModifier()->m_auraname < TOTAL_AURAS)
        m_modAuras[aura->GetModifier()->m_auraname].push_back(aura);
}

void Unit::RemoveRankAurasDueToSpell(uint32 spellId)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if(!spellInfo)
        return;
    SpellAuraHolderMap::const_iterator i,next;
    for (i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); i = next)
    {
        next = i;
        ++next;
        uint32 i_spellId = (*i).second->GetId();
        if((*i).second && i_spellId && i_spellId != spellId)
        {
            if(sSpellMgr.IsRankSpellDueToSpell(spellInfo,i_spellId))
            {
                RemoveAurasDueToSpell(i_spellId);

                if( m_spellAuraHolders.empty() )
                    break;
                else
                    next =  m_spellAuraHolders.begin();
            }
        }
    }
}

bool Unit::RemoveNoStackAurasDueToAuraHolder(SpellAuraHolder *holder)
{
    if (!holder)
        return false;

    SpellEntry const* spellProto = holder->GetSpellProto();
    if (!spellProto)
        return false;

    uint32 spellId = holder->GetId();

    // passive spell special case (only non stackable with ranks)
    if(IsPassiveSpell(spellProto))
    {
        if(IsPassiveSpellStackableWithRanks(spellProto))
            return true;
    }

    SpellSpecific spellId_spec = GetSpellSpecific(spellId);

    SpellAuraHolderMap::iterator i,next;
    for (i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); i = next)
    {
        next = i;
        ++next;
        if (!(*i).second) continue;

        SpellEntry const* i_spellProto = (*i).second->GetSpellProto();

        if (!i_spellProto)
            continue;

        uint32 i_spellId = i_spellProto->Id;

        // early checks that spellId is passive non stackable spell
        if (IsPassiveSpell(i_spellProto))
        {
            // passive non-stackable spells not stackable only for same caster
            if (holder->GetCasterGuid() != i->second->GetCasterGuid())
                continue;

            // passive non-stackable spells not stackable only with another rank of same spell
            if (!sSpellMgr.IsRankSpellDueToSpell(spellProto, i_spellId))
                continue;
        }

        if(i_spellId == spellId) continue;

        bool is_triggered_by_spell = false;
        // prevent triggering aura of removing aura that triggered it
        for(int j = 0; j < MAX_EFFECT_INDEX; ++j)
            if (i_spellProto->EffectTriggerSpell[j] == spellId)
                is_triggered_by_spell = true;

        // prevent triggered aura of removing aura that triggering it (triggered effect early some aura of parent spell
        for(int j = 0; j < MAX_EFFECT_INDEX; ++j)
            if (spellProto->EffectTriggerSpell[j] == i_spellId)
                is_triggered_by_spell = true;

        if (is_triggered_by_spell)
            continue;

        SpellSpecific i_spellId_spec = GetSpellSpecific(i_spellId);

        // single allowed spell specific from same caster or from any caster at target
        bool is_spellSpecPerTargetPerCaster = IsSingleFromSpellSpecificPerTargetPerCaster(spellId_spec,i_spellId_spec);
        bool is_spellSpecPerTarget = IsSingleFromSpellSpecificPerTarget(spellId_spec,i_spellId_spec);
        if (is_spellSpecPerTarget || (is_spellSpecPerTargetPerCaster && holder->GetCasterGUID() == (*i).second->GetCasterGUID()))
        {
            // cannot remove higher rank
            if (sSpellMgr.IsRankSpellDueToSpell(spellProto, i_spellId))
                if(CompareAuraRanks(spellId, i_spellId) < 0)
                    return false;

            // Its a parent aura (create this aura in ApplyModifier)
            if ((*i).second->IsInUse())
            {
                sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                continue;
            }
            RemoveAurasDueToSpell(i_spellId);

            if( m_spellAuraHolders.empty() )
                break;
            else
                next =  m_spellAuraHolders.begin();

            continue;
        }

        // spell with spell specific that allow single ranks for spell from diff caster
        // same caster case processed or early or later
        bool is_spellPerTarget = IsSingleFromSpellSpecificSpellRanksPerTarget(spellId_spec,i_spellId_spec);
        if ( is_spellPerTarget && holder->GetCasterGUID() != (*i).second->GetCasterGUID() && sSpellMgr.IsRankSpellDueToSpell(spellProto, i_spellId))
        {
            // cannot remove higher rank
            if(CompareAuraRanks(spellId, i_spellId) < 0)
                return false;

            // Its a parent aura (create this aura in ApplyModifier)
            if ((*i).second->IsInUse())
            {
                sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                continue;
            }
            RemoveAurasDueToSpell(i_spellId);

            if( m_spellAuraHolders.empty() )
                break;
            else
                next =  m_spellAuraHolders.begin();

            continue;
        }

        // non single (per caster) per target spell specific (possible single spell per target at caster)
        if( !is_spellSpecPerTargetPerCaster && !is_spellSpecPerTarget && sSpellMgr.IsNoStackSpellDueToSpell(spellId, i_spellId) )
        {
            // Its a parent aura (create this aura in ApplyModifier)
            if ((*i).second->IsInUse())
            {
                sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                continue;
            }
            RemoveAurasDueToSpell(i_spellId);

            if( m_spellAuraHolders.empty() )
                break;
            else
                next =  m_spellAuraHolders.begin();

            continue;
        }

        // Potions stack aura by aura (elixirs/flask already checked)
        if( spellProto->SpellFamilyName == SPELLFAMILY_POTION && i_spellProto->SpellFamilyName == SPELLFAMILY_POTION )
        {
            if (IsNoStackAuraDueToAura(spellId, i_spellId))
            {
                if(CompareAuraRanks(spellId, i_spellId) < 0)
                    return false;                       // cannot remove higher rank

                // Its a parent aura (create this aura in ApplyModifier)
                if ((*i).second->IsInUse())
                {
                    sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                    continue;
                }
                RemoveAurasDueToSpell(i_spellId);

                if( m_spellAuraHolders.empty() )
                    break;
                else
                    next =  m_spellAuraHolders.begin();
            }
        }
    }
    return true;
}

void Unit::RemoveAura(uint32 spellId, SpellEffectIndex effindex, Aura* except)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for(SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second; )
    {
        Aura *aur = iter->second->m_auras[effindex];
        if (aur && aur != except)
        {
            RemoveSingleAuraFromSpellAuraHolder(iter->second, effindex);
            // may remove holder
            spair = GetSpellAuraHolderBounds(spellId);
            iter = spair.first;
        }
        else
            ++iter;
    }
}
void Unit::RemoveAurasByCasterSpell(uint32 spellId, uint64 casterGUID)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for(SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second; )
    {
        if (iter->second->GetCasterGUID() == casterGUID)
        {
            RemoveSpellAuraHolder(iter->second);
            spair = GetSpellAuraHolderBounds(spellId);
            iter = spair.first;
        }
        else
            ++iter;
    }
}

void Unit::RemoveSingleAuraFromSpellAuraHolder(uint32 spellId, SpellEffectIndex effindex, uint64 casterGUID, AuraRemoveMode mode)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for(SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second; )
    {
        Aura *aur = iter->second->m_auras[effindex];
        if (aur && aur->GetCasterGUID() == casterGUID)
        {
            RemoveSingleAuraFromSpellAuraHolder(iter->second, effindex, mode);
            spair = GetSpellAuraHolderBounds(spellId);
            iter = spair.first;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAuraHolderDueToSpellByDispel(uint32 spellId, int32 stackAmount, uint64 casterGUID, Unit *dispeler)
{
    SpellEntry const* spellEntry = sSpellStore.LookupEntry(spellId);

    // Custom dispel cases
    // Unstable Affliction
    if(spellEntry->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellEntry->SpellFamilyFlags & UI64LIT(0x010000000000)))
    {
        if (Aura* dotAura = GetAura(SPELL_AURA_PERIODIC_DAMAGE,SPELLFAMILY_WARLOCK,UI64LIT(0x010000000000),0x00000000,casterGUID))
        {
            // use spellpower-modified value for initial damage
            int damage = dotAura->GetModifier()->m_amount;
            damage *= 9;

            // Remove spell auras from stack
            RemoveAuraHolderFromStack(spellId, stackAmount, casterGUID, AURA_REMOVE_BY_DISPEL);

            // backfire damage and silence
            dispeler->CastCustomSpell(dispeler, 31117, &damage, NULL, NULL, true, NULL, NULL,casterGUID);
            return;
        }
    }
    // Lifebloom
    else if (spellEntry->SpellFamilyName == SPELLFAMILY_DRUID && (spellEntry->SpellFamilyFlags & UI64LIT(0x0000001000000000)))
    {
        if (Aura* dotAura = GetAura(SPELL_AURA_DUMMY, SPELLFAMILY_DRUID, UI64LIT(0x0000001000000000), 0x00000000, casterGUID))
        {
            int32 amount = ( dotAura->GetModifier()->m_amount / dotAura->GetStackAmount() ) * stackAmount;
            CastCustomSpell(this, 33778, &amount, NULL, NULL, true, NULL, dotAura, casterGUID);

            if (Unit* caster = dotAura->GetCaster())
            {
                int32 returnmana = (spellEntry->ManaCostPercentage * caster->GetCreateMana() / 100) * stackAmount / 2;
                caster->CastCustomSpell(caster, 64372, &returnmana, NULL, NULL, true, NULL, dotAura, casterGUID);
            }
        }
    }
    // Flame Shock
    else if (spellEntry->SpellFamilyName == SPELLFAMILY_SHAMAN && (spellEntry->SpellFamilyFlags & UI64LIT(0x10000000)))
    {
        Unit* caster = NULL;
        uint32 triggeredSpell = 0;

        if (Aura* dotAura = GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN, UI64LIT(0x10000000), 0x00000000, casterGUID))
            caster = dotAura->GetCaster();

        if (caster && !caster->isDead())
        {
            Unit::AuraList const& auras = caster->GetAurasByType(SPELL_AURA_DUMMY);
            for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
            {
                switch((*i)->GetId())
                {
                    case 51480: triggeredSpell=64694; break;// Lava Flows, Rank 1
                    case 51481: triggeredSpell=65263; break;// Lava Flows, Rank 2
                    case 51482: triggeredSpell=65264; break;// Lava Flows, Rank 3
                    default: continue;
                }
                break;
            }
        }

        // Remove spell auras from stack
        RemoveAuraHolderFromStack(spellId, stackAmount, casterGUID, AURA_REMOVE_BY_DISPEL);

        // Haste
        if (triggeredSpell)
            caster->CastSpell(caster, triggeredSpell, true);
        return;
    }
    // Vampiric touch (first dummy aura)
    else if (spellEntry->SpellFamilyName == SPELLFAMILY_PRIEST && spellEntry->SpellFamilyFlags & UI64LIT(0x0000040000000000))
    {
        if (Aura *dot = GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, UI64LIT(0x0000040000000000), 0x00000000, casterGUID))
        {
            if (dot->GetCaster())
            {
                // use clean value for initial damage
                int32 bp0 = dot->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1);
                bp0 *= 8;

                // Remove spell auras from stack
                RemoveAuraHolderFromStack(spellId, stackAmount, casterGUID, AURA_REMOVE_BY_DISPEL);

                CastCustomSpell(this, 64085, &bp0, NULL, NULL, true, NULL, NULL, casterGUID);
                return;
            }
        }
    }

    RemoveAuraHolderFromStack(spellId, stackAmount, casterGUID, AURA_REMOVE_BY_DISPEL);
}

void Unit::RemoveAurasDueToSpellBySteal(uint32 spellId, uint64 casterGUID, Unit *stealer)
{
    SpellAuraHolder *holder = GetSpellAuraHolder(spellId, casterGUID);
    SpellEntry const* spellProto = sSpellStore.LookupEntry(spellId);
    SpellAuraHolder *new_holder = CreateSpellAuraHolder(spellProto, stealer, this);

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        Aura *aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i));

        if (!aur)
            continue;

        int32 basePoints = aur->GetBasePoints();
        // construct the new aura for the attacker - will never return NULL, it's just a wrapper for
        // some different constructors
        Aura * new_aur = CreateAura(aur->GetSpellProto(), aur->GetEffIndex(), &basePoints, new_holder, stealer, this);

        // set its duration and maximum duration
        // max duration 2 minutes (in msecs)
        int32 dur = aur->GetAuraDuration();
        int32 max_dur = 2*MINUTE*IN_MILLISECONDS;
        int32 new_max_dur = max_dur > dur ? dur : max_dur;
        new_aur->SetAuraMaxDuration( new_max_dur );
        new_aur->SetAuraDuration( new_max_dur );

        // set periodic to do at least one tick (for case when original aura has been at last tick preparing)
        int32 periodic = aur->GetModifier()->periodictime;
        new_aur->GetModifier()->periodictime = periodic < new_max_dur ? periodic : new_max_dur;

        // add the new aura to stealer
        new_holder->AddAura(new_aur, new_aur->GetEffIndex());
    }

    if (holder->ModStackAmount(-1))
        // Remove aura as dispel
        RemoveSpellAuraHolder(holder, AURA_REMOVE_BY_DISPEL);

    // strange but intended behaviour: Stolen single target auras won't be treated as single targeted
    new_holder->SetIsSingleTarget(false);

    stealer->AddSpellAuraHolder(new_holder);
}

void Unit::RemoveAurasDueToSpellByCancel(uint32 spellId)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for(SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second;)
    {
        RemoveSpellAuraHolder(iter->second, AURA_REMOVE_BY_CANCEL);
        spair = GetSpellAuraHolderBounds(spellId);
        iter = spair.first;
    }
}

void Unit::RemoveAurasWithDispelType( DispelType type, uint64 casterGUID )
{
    // Create dispel mask by dispel type
    uint32 dispelMask = GetDispellMask(type);
    // Dispel all existing auras vs current dispel type
    SpellAuraHolderMap& auras = GetSpellAuraHolderMap();
    for(SpellAuraHolderMap::iterator itr = auras.begin(); itr != auras.end(); )
    {
        SpellEntry const* spell = itr->second->GetSpellProto();
        if( ((1<<spell->Dispel) & dispelMask) && (!casterGUID || casterGUID == itr->second->GetCasterGUID()))
        {
            // Dispel aura
            RemoveAurasDueToSpell(spell->Id);
            itr = auras.begin();
        }
        else
            ++itr;
    }
}

void Unit::RemoveAuraHolderFromStack(uint32 spellId, int32 stackAmount, uint64 casterGUID, AuraRemoveMode mode)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for(SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second; ++iter)
    {
        if (!casterGUID || iter->second->GetCasterGUID() == casterGUID)
        {
            if (iter->second->GetAuraCharges() > 1)
            {
                while (stackAmount--)
                {
                    if (iter->second->DropAuraCharge())
                    {
                        RemoveSpellAuraHolder(iter->second, mode);
                        break;
                    }
                }
            }
            else
            {
                if (iter->second->ModStackAmount(-stackAmount))
                {
                    RemoveSpellAuraHolder(iter->second, mode);
                    break;
                }
            }
        }
    }
}

void Unit::RemoveAurasDueToSpell(uint32 spellId, SpellAuraHolder* except, AuraRemoveMode mode)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = bounds.first; iter != bounds.second; )
    {
        if (iter->second != except)
        {
            RemoveSpellAuraHolder(iter->second, mode);
            bounds = GetSpellAuraHolderBounds(spellId);
            iter = bounds.first;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasDueToItemSpell(Item* castItem,uint32 spellId)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = bounds.first; iter != bounds.second; )
    {
        if (iter->second->GetCastItemGuid() == castItem->GetObjectGuid())
        {
            RemoveSpellAuraHolder(iter->second);
            bounds = GetSpellAuraHolderBounds(spellId);
            iter = bounds.first;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasWithInterruptFlags(uint32 flags)
{
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end(); )
    {
        if (iter->second->GetSpellProto()->AuraInterruptFlags & flags)
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasWithAttribute(uint32 flags)
{
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end(); )
    {
        if (iter->second->GetSpellProto()->Attributes & flags)
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
            ++iter;
    }
}

void Unit::RemoveNotOwnSingleTargetAuras(uint32 newPhase)
{
    // single target auras from other casters
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end(); )
    {
        if (iter->second->GetCasterGuid() != GetObjectGuid() && iter->second->IsSingleTarget())
        {
            if (!newPhase)
            {
                RemoveSpellAuraHolder(iter->second);
                iter = m_spellAuraHolders.begin();
                continue;
            }
            else
            {
                Unit* caster = iter->second->GetCaster();
                if (!caster || !caster->InSamePhase(newPhase))
                {
                    RemoveSpellAuraHolder(iter->second);
                    iter = m_spellAuraHolders.begin();
                    continue;
                }
            }
        }

        ++iter;
    }

    // single target auras at other targets
    SingleCastSpellTargetMap& scTargets = GetSingleCastSpellTargets();
    for (SingleCastSpellTargetMap::iterator itr = scTargets.begin(); itr != scTargets.end(); )
    {
        SpellEntry const* itr_spellEntry = itr->first;
        ObjectGuid itr_targetGuid = itr->second;

        if (itr_targetGuid != GetObjectGuid())
        {
            if(!newPhase)
            {
                scTargets.erase(itr);                       // remove for caster in any case

                // remove from target if target found
                if (Unit* itr_target = GetMap()->GetUnit(itr_targetGuid))
                    itr_target->RemoveAurasByCasterSpell(itr_spellEntry->Id, GetGUID());

                itr = scTargets.begin();                    // list can be changed at remove aura
                continue;
            }
            else
            {
                Unit* itr_target = GetMap()->GetUnit(itr_targetGuid);
                if(!itr_target || !itr_target->InSamePhase(newPhase))
                {
                    scTargets.erase(itr);                   // remove for caster in any case

                    // remove from target if target found
                    if (itr_target)
                        itr_target->RemoveAurasByCasterSpell(itr_spellEntry->Id, GetGUID());

                    itr = scTargets.begin();                // list can be changed at remove aura
                    continue;
                }
            }
        }

        ++itr;
    }

}

void Unit::RemoveSpellAuraHolder(SpellAuraHolder *holder, AuraRemoveMode mode)
{
    // Statue unsummoned at holder remove
    SpellEntry const* AurSpellInfo = holder->GetSpellProto();
    Totem* statue = NULL;
    Unit* caster = holder->GetCaster();
    if(IsChanneledSpell(AurSpellInfo) && caster)
        if(caster->GetTypeId()==TYPEID_UNIT && ((Creature*)caster)->IsTotem() && ((Totem*)caster)->GetTotemType()==TOTEM_STATUE)
            statue = ((Totem*)caster);

    if (m_spellAuraHoldersUpdateIterator != m_spellAuraHolders.end() && m_spellAuraHoldersUpdateIterator->second == holder)
        ++m_spellAuraHoldersUpdateIterator;

    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(holder->GetId());
    for (SpellAuraHolderMap::iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        if (itr->second == holder)
        {
            m_spellAuraHolders.erase(itr);
            break;
        }
    }

    holder->SetRemoveMode(mode);
    holder->UnregisterSingleCastHolder();

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (Aura *aura = holder->m_auras[i])
            RemoveAura(aura, mode);
    }

    holder->_RemoveSpellAuraHolder();

    if (mode != AURA_REMOVE_BY_DELETE)
        holder->HandleSpellSpecificBoosts(false);

    if(statue)
        statue->UnSummon();

    // If holder in use (removed from code that plan access to it data after return)
    // store it in holder list with delayed deletion
    if (holder->IsInUse())
    {
        holder->SetDeleted();
        m_deletedHolders.push_back(holder);
    }
    else
        delete holder;

    if (mode != AURA_REMOVE_BY_EXPIRE && IsChanneledSpell(AurSpellInfo) && !IsAreaOfEffectSpell(AurSpellInfo) && caster && caster->GetGUID() != GetGUID())
        caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
}

void Unit::RemoveSingleAuraFromSpellAuraHolder(SpellAuraHolder *holder, SpellEffectIndex index, AuraRemoveMode mode)
{
    Aura *aura = holder->GetAuraByEffectIndex(index);
    if (!aura)
        return;

    if (aura->IsLastAuraOnHolder())
        RemoveSpellAuraHolder(holder, mode);
    else
        RemoveAura(aura, mode);
}

void Unit::RemoveAura(Aura *Aur, AuraRemoveMode mode)
{
    // remove from list before mods removing (prevent cyclic calls, mods added before including to aura list - use reverse order)
    if (Aur->GetModifier()->m_auraname < TOTAL_AURAS)
    {
        m_modAuras[Aur->GetModifier()->m_auraname].remove(Aur);
    }

    // Set remove mode
    Aur->SetRemoveMode(mode);

    // some ShapeshiftBoosts at remove trigger removing other auras including parent Shapeshift aura
    // remove aura from list before to prevent deleting it before
    ///m_Auras.erase(i);

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Aura %u now is remove mode %d",Aur->GetModifier()->m_auraname, mode);

    // aura _MUST_ be remove from holder before unapply.
    // un-apply code expected that aura not find by diff searches
    // in another case it can be double removed for example, if target die/etc in un-apply process.
    Aur->GetHolder()->RemoveAura(Aur->GetEffIndex());

    // some auras also need to apply modifier (on caster) on remove
    if (mode == AURA_REMOVE_BY_DELETE)
    {
        switch (Aur->GetModifier()->m_auraname)
        {
            // need properly undo any auras with player-caster mover set (or will crash at next caster move packet)
            case SPELL_AURA_MOD_POSSESS:
            case SPELL_AURA_MOD_POSSESS_PET:
            case SPELL_AURA_CONTROL_VEHICLE:
                Aur->ApplyModifier(false,true);
                break;
            default: break;
        }
    }
    else
        Aur->ApplyModifier(false,true);

    // If aura in use (removed from code that plan access to it data after return)
    // store it in aura list with delayed deletion
    if (Aur->IsInUse())
        m_deletedAuras.push_back(Aur);
    else
        delete Aur;
}

void Unit::RemoveAllAuras(AuraRemoveMode mode /*= AURA_REMOVE_BY_DEFAULT*/)
{
    while (!m_spellAuraHolders.empty())
    {
        SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin();
        RemoveSpellAuraHolder(iter->second,mode);
    }
}

void Unit::RemoveArenaAuras(bool onleave)
{
    // in join, remove positive buffs, on end, remove negative
    // used to remove positive visible auras in arenas
    for(SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        if (!(iter->second->GetSpellProto()->AttributesEx4 & SPELL_ATTR_EX4_UNK21) &&
                                                            // don't remove stances, shadowform, pally/hunter auras
            !iter->second->IsPassive() &&                   // don't remove passive auras
            (!(iter->second->GetSpellProto()->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY) ||
            !(iter->second->GetSpellProto()->Attributes & SPELL_ATTR_UNK8)) &&
                                                            // not unaffected by invulnerability auras or not having that unknown flag (that seemed the most probable)
            (iter->second->IsPositive() != onleave))        // remove positive buffs on enter, negative buffs on leave
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
            ++iter;
    }
}

void Unit::RemoveAllAurasOnDeath()
{
    // used just after dieing to remove all visible auras
    // and disable the mods for the passive ones
    for(SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        if (!iter->second->IsPassive() && !iter->second->IsDeathPersistent())
        {
            RemoveSpellAuraHolder(iter->second, AURA_REMOVE_BY_DEATH);
            iter = m_spellAuraHolders.begin();
        }
        else
            ++iter;
    }
}

void Unit::DelaySpellAuraHolder(uint32 spellId, int32 delaytime, uint64 casterGUID)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = bounds.first; iter != bounds.second; ++iter)
    {
        if (casterGUID != iter->second->GetCasterGUID())
            continue;

        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (Aura *aur = iter->second->GetAuraByEffectIndex(SpellEffectIndex(i)))
            {
                if (aur->GetAuraDuration() < delaytime)
                    aur->SetAuraDuration(0);
                else
                    aur->SetAuraDuration(aur->GetAuraDuration() - delaytime);

                DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell %u, EffectIndex %u partially interrupted on unit %u, new duration: %u ms",spellId, i, GetGUIDLow(), aur->GetAuraDuration());
            }
        }
        iter->second->SendAuraUpdate(false);
    }
}

void Unit::_RemoveAllAuraMods()
{
    for (SpellAuraHolderMap::const_iterator i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); ++i)
    {
        (*i).second->ApplyAuraModifiers(false);
    }
}

void Unit::_ApplyAllAuraMods()
{
    for (SpellAuraHolderMap::const_iterator i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); ++i)
    {
        (*i).second->ApplyAuraModifiers(true);
    }
}

Aura* Unit::GetAura(uint32 spellId, SpellEffectIndex effindex)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    if (bounds.first != bounds.second)
        return bounds.first->second->GetAuraByEffectIndex(effindex);
    return NULL;
}

Aura* Unit::GetAura(AuraType type, SpellFamily family, uint64 familyFlag, uint32 familyFlag2, ObjectGuid casterGuid)
{
    AuraList const& auras = GetAurasByType(type);
    for(AuraList::const_iterator i = auras.begin();i != auras.end(); ++i)
        if ((*i)->GetSpellProto()->IsFitToFamily(family, familyFlag, familyFlag2) &&
            (casterGuid.IsEmpty() || (*i)->GetCasterGuid() == casterGuid))
            return *i;

    return NULL;
}

bool Unit::HasAura(uint32 spellId, SpellEffectIndex effIndex) const
{
    SpellAuraHolderConstBounds spair = GetSpellAuraHolderBounds(spellId);
    for(SpellAuraHolderMap::const_iterator i_holder = spair.first; i_holder != spair.second; ++i_holder)
        if (i_holder->second->GetAuraByEffectIndex(effIndex))
            return true;

    return false;
}

void Unit::AddDynObject(DynamicObject* dynObj)
{
    m_dynObjGUIDs.push_back(dynObj->GetGUID());
}

void Unit::RemoveDynObject(uint32 spellid)
{
    if(m_dynObjGUIDs.empty())
        return;
    for (DynObjectGUIDs::iterator i = m_dynObjGUIDs.begin(); i != m_dynObjGUIDs.end();)
    {
        DynamicObject* dynObj = GetMap()->GetDynamicObject(*i);
        if(!dynObj)
        {
            i = m_dynObjGUIDs.erase(i);
        }
        else if(spellid == 0 || dynObj->GetSpellId() == spellid)
        {
            dynObj->Delete();
            i = m_dynObjGUIDs.erase(i);
        }
        else
            ++i;
    }
}

void Unit::RemoveAllDynObjects()
{
    while(!m_dynObjGUIDs.empty())
    {
        DynamicObject* dynObj = GetMap()->GetDynamicObject(*m_dynObjGUIDs.begin());
        if(dynObj)
            dynObj->Delete();
        m_dynObjGUIDs.erase(m_dynObjGUIDs.begin());
    }
}

DynamicObject * Unit::GetDynObject(uint32 spellId, SpellEffectIndex effIndex)
{
    for (DynObjectGUIDs::iterator i = m_dynObjGUIDs.begin(); i != m_dynObjGUIDs.end();)
    {
        DynamicObject* dynObj = GetMap()->GetDynamicObject(*i);
        if(!dynObj)
        {
            i = m_dynObjGUIDs.erase(i);
            continue;
        }

        if (dynObj->GetSpellId() == spellId && dynObj->GetEffIndex() == effIndex)
            return dynObj;
        ++i;
    }
    return NULL;
}

DynamicObject * Unit::GetDynObject(uint32 spellId)
{
    for (DynObjectGUIDs::iterator i = m_dynObjGUIDs.begin(); i != m_dynObjGUIDs.end();)
    {
        DynamicObject* dynObj = GetMap()->GetDynamicObject(*i);
        if(!dynObj)
        {
            i = m_dynObjGUIDs.erase(i);
            continue;
        }

        if (dynObj->GetSpellId() == spellId)
            return dynObj;
        ++i;
    }
    return NULL;
}

GameObject* Unit::GetGameObject(uint32 spellId) const
{
    for (GameObjectList::const_iterator i = m_gameObj.begin(); i != m_gameObj.end(); ++i)
        if ((*i)->GetSpellId() == spellId)
            return *i;

    return NULL;
}

void Unit::AddGameObject(GameObject* gameObj)
{
    MANGOS_ASSERT(gameObj && gameObj->GetOwnerGuid().IsEmpty());
    m_gameObj.push_back(gameObj);
    gameObj->SetOwnerGuid(GetObjectGuid());

    if ( GetTypeId()==TYPEID_PLAYER && gameObj->GetSpellId() )
    {
        SpellEntry const* createBySpell = sSpellStore.LookupEntry(gameObj->GetSpellId());
        // Need disable spell use for owner
        if (createBySpell && createBySpell->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
            // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
            ((Player*)this)->AddSpellAndCategoryCooldowns(createBySpell,0,NULL,true);
    }
}

void Unit::RemoveGameObject(GameObject* gameObj, bool del)
{
    MANGOS_ASSERT(gameObj && gameObj->GetOwnerGuid() == GetObjectGuid());

    gameObj->SetOwnerGuid(ObjectGuid());

    // GO created by some spell
    if (uint32 spellid = gameObj->GetSpellId())
    {
        RemoveAurasDueToSpell(spellid);

        if (GetTypeId()==TYPEID_PLAYER)
        {
            SpellEntry const* createBySpell = sSpellStore.LookupEntry(spellid );
            // Need activate spell use for owner
            if (createBySpell && createBySpell->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
                ((Player*)this)->SendCooldownEvent(createBySpell);
        }
    }

    m_gameObj.remove(gameObj);

    if(del)
    {
        gameObj->SetRespawnTime(0);
        gameObj->Delete();
    }
}

void Unit::RemoveGameObject(uint32 spellid, bool del)
{
    if(m_gameObj.empty())
        return;
    GameObjectList::iterator i, next;
    for (i = m_gameObj.begin(); i != m_gameObj.end(); i = next)
    {
        next = i;
        if(spellid == 0 || (*i)->GetSpellId() == spellid)
        {
            (*i)->SetOwnerGuid(ObjectGuid());
            if(del)
            {
                (*i)->SetRespawnTime(0);
                (*i)->Delete();
            }

            next = m_gameObj.erase(i);
        }
        else
            ++next;
    }
}

void Unit::RemoveAllGameObjects()
{
    // remove references to unit
    for(GameObjectList::iterator i = m_gameObj.begin(); i != m_gameObj.end();)
    {
        (*i)->SetOwnerGuid(ObjectGuid());
        (*i)->SetRespawnTime(0);
        (*i)->Delete();
        i = m_gameObj.erase(i);
    }
}

void Unit::SendSpellNonMeleeDamageLog(SpellNonMeleeDamage *log)
{
    uint32 targetHealth = log->target->GetHealth();
    uint32 overkill = log->damage > targetHealth ? log->damage - targetHealth : 0;

    WorldPacket data(SMSG_SPELLNONMELEEDAMAGELOG, (16+4+4+4+1+4+4+1+1+4+4+1)); // we guess size
    data << log->target->GetPackGUID();
    data << log->attacker->GetPackGUID();
    data << uint32(log->SpellID);
    data << uint32(log->damage);                            // damage amount
    data << uint32(overkill);                               // overkill
    data << uint8 (log->schoolMask);                        // damage school
    data << uint32(log->absorb);                            // AbsorbedDamage
    data << uint32(log->resist);                            // resist
    data << uint8 (log->physicalLog);                       // if 1, then client show spell name (example: %s's ranged shot hit %s for %u school or %s suffers %u school damage from %s's spell_name
    data << uint8 (log->unused);                            // unused
    data << uint32(log->blocked);                           // blocked
    data << uint32(log->HitInfo);
    data << uint8 (0);                                      // flag to use extend data
    SendMessageToSet( &data, true );
}

void Unit::SendSpellNonMeleeDamageLog(Unit *target, uint32 SpellID, uint32 Damage, SpellSchoolMask damageSchoolMask, uint32 AbsorbedDamage, uint32 Resist, bool PhysicalDamage, uint32 Blocked, bool CriticalHit)
{
    SpellNonMeleeDamage log(this, target, SpellID, damageSchoolMask);
    log.damage = Damage - AbsorbedDamage - Resist - Blocked;
    log.absorb = AbsorbedDamage;
    log.resist = Resist;
    log.physicalLog = PhysicalDamage;
    log.blocked = Blocked;
    log.HitInfo = SPELL_HIT_TYPE_UNK1 | SPELL_HIT_TYPE_UNK3 | SPELL_HIT_TYPE_UNK6;
    if(CriticalHit)
        log.HitInfo |= SPELL_HIT_TYPE_CRIT;
    SendSpellNonMeleeDamageLog(&log);
}

void Unit::SendPeriodicAuraLog(SpellPeriodicAuraLogInfo *pInfo)
{
    Aura *aura = pInfo->aura;
    Modifier *mod = aura->GetModifier();

    WorldPacket data(SMSG_PERIODICAURALOG, 30);
    data << aura->GetTarget()->GetPackGUID();
    data << aura->GetCasterGuid().WriteAsPacked();
    data << uint32(aura->GetId());                          // spellId
    data << uint32(1);                                      // count
    data << uint32(mod->m_auraname);                        // auraId
    switch(mod->m_auraname)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
            data << uint32(pInfo->damage);                  // damage
            data << uint32(pInfo->overDamage);              // overkill?
            data << uint32(GetSpellSchoolMask(aura->GetSpellProto()));
            data << uint32(pInfo->absorb);                  // absorb
            data << uint32(pInfo->resist);                  // resist
            data << uint8(pInfo->critical ? 1 : 0);         // new 3.1.2 critical flag
            break;
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
            data << uint32(pInfo->damage);                  // damage
            data << uint32(pInfo->overDamage);              // overheal?
            data << uint32(pInfo->absorb);                  // absorb
            data << uint8(pInfo->critical ? 1 : 0);         // new 3.1.2 critical flag
            break;
        case SPELL_AURA_OBS_MOD_MANA:
        case SPELL_AURA_PERIODIC_ENERGIZE:
            data << uint32(mod->m_miscvalue);               // power type
            data << uint32(pInfo->damage);                  // damage
            break;
        case SPELL_AURA_PERIODIC_MANA_LEECH:
            data << uint32(mod->m_miscvalue);               // power type
            data << uint32(pInfo->damage);                  // amount
            data << float(pInfo->multiplier);               // gain multiplier
            break;
        default:
            sLog.outError("Unit::SendPeriodicAuraLog: unknown aura %u", uint32(mod->m_auraname));
            return;
    }

    aura->GetTarget()->SendMessageToSet(&data, true);
}

void Unit::ProcDamageAndSpell(Unit *pVictim, uint32 procAttacker, uint32 procVictim, uint32 procExtra, uint32 amount, WeaponAttackType attType, SpellEntry const *procSpell)
{
     // Not much to do if no flags are set.
    if (procAttacker)
        ProcDamageAndSpellFor(false,pVictim,procAttacker, procExtra,attType, procSpell, amount);
    // Now go on with a victim's events'n'auras
    // Not much to do if no flags are set or there is no victim
    if(pVictim && pVictim->isAlive() && procVictim)
        pVictim->ProcDamageAndSpellFor(true,this,procVictim, procExtra, attType, procSpell, amount);
}

void Unit::SendSpellMiss(Unit *target, uint32 spellID, SpellMissInfo missInfo)
{
    WorldPacket data(SMSG_SPELLLOGMISS, (4+8+1+4+8+1));
    data << uint32(spellID);
    data << GetObjectGuid();
    data << uint8(0);                                       // can be 0 or 1
    data << uint32(1);                                      // target count
    // for(i = 0; i < target count; ++i)
    data << target->GetObjectGuid();                        // target GUID
    data << uint8(missInfo);
    // end loop
    SendMessageToSet(&data, true);
}

void Unit::SendAttackStateUpdate(CalcDamageInfo *damageInfo)
{
    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "WORLD: Sending SMSG_ATTACKERSTATEUPDATE");

    uint32 targetHealth = damageInfo->target->GetHealth();
    uint32 overkill = damageInfo->damage > targetHealth ? damageInfo->damage - targetHealth : 0;

    uint32 count = 1;
    WorldPacket data(SMSG_ATTACKERSTATEUPDATE, 16 + 45);    // we guess size
    data << uint32(damageInfo->HitInfo);
    data << damageInfo->attacker->GetPackGUID();
    data << damageInfo->target->GetPackGUID();
    data << uint32(damageInfo->damage);                     // Full damage
    data << uint32(overkill);                               // overkill value
    data << uint8(count);                                   // Sub damage count

    for(uint32 i = 0; i < count; ++i)
    {
        data << uint32(damageInfo->damageSchoolMask);       // School of sub damage
        data << float(damageInfo->damage);                  // sub damage
        data << uint32(damageInfo->damage);                 // Sub Damage
    }

    if(damageInfo->HitInfo & (HITINFO_ABSORB | HITINFO_ABSORB2))
    {
        for(uint32 i = 0; i < count; ++i)
            data << uint32(damageInfo->absorb);             // Absorb
    }

    if(damageInfo->HitInfo & (HITINFO_RESIST | HITINFO_RESIST2))
    {
        for(uint32 i = 0; i < count; ++i)
            data << uint32(damageInfo->resist);             // Resist
    }

    data << uint8(damageInfo->TargetState);
    data << uint32(0);
    data << uint32(0);

    if(damageInfo->HitInfo & HITINFO_BLOCK)
        data << uint32(damageInfo->blocked_amount);

    if(damageInfo->HitInfo & HITINFO_UNK3)
        data << uint32(0);

    if(damageInfo->HitInfo & HITINFO_UNK1)
    {
        data << uint32(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        for(uint8 i = 0; i < 5; ++i)
        {
            data << float(0);
            data << float(0);
        }
        data << uint32(0);
    }

    SendMessageToSet( &data, true );
}

void Unit::SendAttackStateUpdate(uint32 HitInfo, Unit *target, uint8 /*SwingType*/, SpellSchoolMask damageSchoolMask, uint32 Damage, uint32 AbsorbDamage, uint32 Resist, VictimState TargetState, uint32 BlockedAmount)
{
    CalcDamageInfo dmgInfo;
    dmgInfo.HitInfo = HitInfo;
    dmgInfo.attacker = this;
    dmgInfo.target = target;
    dmgInfo.damage = Damage - AbsorbDamage - Resist - BlockedAmount;
    dmgInfo.damageSchoolMask = damageSchoolMask;
    dmgInfo.absorb = AbsorbDamage;
    dmgInfo.resist = Resist;
    dmgInfo.TargetState = TargetState;
    dmgInfo.blocked_amount = BlockedAmount;
    SendAttackStateUpdate(&dmgInfo);
}

void Unit::setPowerType(Powers new_powertype)
{
    SetByteValue(UNIT_FIELD_BYTES_0, 3, new_powertype);

    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(((Player*)this)->GetGroup())
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POWER_TYPE);
    }
    else if(((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_POWER_TYPE);
        }
    }

    switch(new_powertype)
    {
        default:
        case POWER_MANA:
            break;
        case POWER_RAGE:
            SetMaxPower(POWER_RAGE,GetCreatePowers(POWER_RAGE));
            SetPower(   POWER_RAGE,0);
            break;
        case POWER_FOCUS:
            SetMaxPower(POWER_FOCUS,GetCreatePowers(POWER_FOCUS));
            SetPower(   POWER_FOCUS,GetCreatePowers(POWER_FOCUS));
            break;
        case POWER_ENERGY:
            SetMaxPower(POWER_ENERGY,GetCreatePowers(POWER_ENERGY));
            break;
        case POWER_HAPPINESS:
            SetMaxPower(POWER_HAPPINESS,GetCreatePowers(POWER_HAPPINESS));
            SetPower(POWER_HAPPINESS,GetCreatePowers(POWER_HAPPINESS));
            break;
    }
}

FactionTemplateEntry const* Unit::getFactionTemplateEntry() const
{
    FactionTemplateEntry const* entry = sFactionTemplateStore.LookupEntry(getFaction());
    if(!entry)
    {
        static uint64 guid = 0;                             // prevent repeating spam same faction problem

        if(GetGUID() != guid)
        {
            if(GetTypeId() == TYPEID_PLAYER)
                sLog.outError("Player %s have invalid faction (faction template id) #%u", ((Player*)this)->GetName(), getFaction());
            else
                sLog.outError("Creature (template id: %u) have invalid faction (faction template id) #%u", ((Creature*)this)->GetCreatureInfo()->Entry, getFaction());
            guid = GetGUID();
        }
    }
    return entry;
}

bool Unit::IsHostileTo(Unit const* unit) const
{
    // always non-hostile to self
    if(unit==this)
        return false;

    // always non-hostile to GM in GM mode
    if(unit->GetTypeId()==TYPEID_PLAYER && ((Player const*)unit)->isGameMaster())
        return false;

    // always hostile to enemy
    if(getVictim()==unit || unit->getVictim()==this)
        return true;

    // test pet/charm masters instead pers/charmeds
    Unit const* testerOwner = GetCharmerOrOwner();
    Unit const* targetOwner = unit->GetCharmerOrOwner();

    // always hostile to owner's enemy
    if(testerOwner && (testerOwner->getVictim()==unit || unit->getVictim()==testerOwner))
        return true;

    // always hostile to enemy owner
    if(targetOwner && (getVictim()==targetOwner || targetOwner->getVictim()==this))
        return true;

    // always hostile to owner of owner's enemy
    if(testerOwner && targetOwner && (testerOwner->getVictim()==targetOwner || targetOwner->getVictim()==testerOwner))
        return true;

    Unit const* tester = testerOwner ? testerOwner : this;
    Unit const* target = targetOwner ? targetOwner : unit;

    // always non-hostile to target with common owner, or to owner/pet
    if(tester==target)
        return false;

    // special cases (Duel, etc)
    if(tester->GetTypeId()==TYPEID_PLAYER && target->GetTypeId()==TYPEID_PLAYER)
    {
        Player const* pTester = (Player const*)tester;
        Player const* pTarget = (Player const*)target;

        // Duel
        if(pTester->duel && pTester->duel->opponent == pTarget && pTester->duel->startTime != 0)
            return true;

        // Group
        if(pTester->GetGroup() && pTester->GetGroup()==pTarget->GetGroup())
            return false;

        // Sanctuary
        if(pTarget->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY) && pTester->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY))
            return false;

        // PvP FFA state
        if(pTester->IsFFAPvP() && pTarget->IsFFAPvP())
            return true;

        //= PvP states
        // Green/Blue (can't attack)
        if(pTester->GetTeam()==pTarget->GetTeam())
            return false;

        // Red (can attack) if true, Blue/Yellow (can't attack) in another case
        return pTester->IsPvP() && pTarget->IsPvP();
    }

    // faction base cases
    FactionTemplateEntry const*tester_faction = tester->getFactionTemplateEntry();
    FactionTemplateEntry const*target_faction = target->getFactionTemplateEntry();
    if(!tester_faction || !target_faction)
        return false;

    if(target->isAttackingPlayer() && tester->IsContestedGuard())
        return true;

    // PvC forced reaction and reputation case
    if(tester->GetTypeId()==TYPEID_PLAYER)
    {
        // forced reaction
        if(target_faction->faction)
        {
            if(ReputationRank const* force =((Player*)tester)->GetReputationMgr().GetForcedRankIfAny(target_faction))
                return *force <= REP_HOSTILE;

            // if faction have reputation then hostile state for tester at 100% dependent from at_war state
            if(FactionEntry const* raw_target_faction = sFactionStore.LookupEntry(target_faction->faction))
                if(FactionState const* factionState = ((Player*)tester)->GetReputationMgr().GetState(raw_target_faction))
                    return (factionState->Flags & FACTION_FLAG_AT_WAR);
        }
    }
    // CvP forced reaction and reputation case
    else if(target->GetTypeId()==TYPEID_PLAYER)
    {
        // forced reaction
        if(tester_faction->faction)
        {
            if(ReputationRank const* force = ((Player*)target)->GetReputationMgr().GetForcedRankIfAny(tester_faction))
                return *force <= REP_HOSTILE;

            // apply reputation state
            FactionEntry const* raw_tester_faction = sFactionStore.LookupEntry(tester_faction->faction);
            if(raw_tester_faction && raw_tester_faction->reputationListID >=0 )
                return ((Player const*)target)->GetReputationMgr().GetRank(raw_tester_faction) <= REP_HOSTILE;
        }
    }

    // common faction based case (CvC,PvC,CvP)
    return tester_faction->IsHostileTo(*target_faction);
}

bool Unit::IsFriendlyTo(Unit const* unit) const
{
    // always friendly to self
    if(unit==this)
        return true;

    // always friendly to GM in GM mode
    if(unit->GetTypeId()==TYPEID_PLAYER && ((Player const*)unit)->isGameMaster())
        return true;

    // always non-friendly to enemy
    if(getVictim()==unit || unit->getVictim()==this)
        return false;

    // test pet/charm masters instead pers/charmeds
    Unit const* testerOwner = GetCharmerOrOwner();
    Unit const* targetOwner = unit->GetCharmerOrOwner();

    // always non-friendly to owner's enemy
    if(testerOwner && (testerOwner->getVictim()==unit || unit->getVictim()==testerOwner))
        return false;

    // always non-friendly to enemy owner
    if(targetOwner && (getVictim()==targetOwner || targetOwner->getVictim()==this))
        return false;

    // always non-friendly to owner of owner's enemy
    if(testerOwner && targetOwner && (testerOwner->getVictim()==targetOwner || targetOwner->getVictim()==testerOwner))
        return false;

    Unit const* tester = testerOwner ? testerOwner : this;
    Unit const* target = targetOwner ? targetOwner : unit;

    // always friendly to target with common owner, or to owner/pet
    if(tester==target)
        return true;

    // special cases (Duel)
    if(tester->GetTypeId()==TYPEID_PLAYER && target->GetTypeId()==TYPEID_PLAYER)
    {
        Player const* pTester = (Player const*)tester;
        Player const* pTarget = (Player const*)target;

        // Duel
        if(pTester->duel && pTester->duel->opponent == target && pTester->duel->startTime != 0)
            return false;

        // Group
        if(pTester->GetGroup() && pTester->GetGroup()==pTarget->GetGroup())
            return true;

        // Sanctuary
        if(pTarget->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY) && pTester->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY))
            return true;

        // PvP FFA state
        if(pTester->IsFFAPvP() && pTarget->IsFFAPvP())
            return false;

        //= PvP states
        // Green/Blue (non-attackable)
        if(pTester->GetTeam()==pTarget->GetTeam())
            return true;

        // Blue (friendly/non-attackable) if not PVP, or Yellow/Red in another case (attackable)
        return !pTarget->IsPvP();
    }

    // faction base cases
    FactionTemplateEntry const*tester_faction = tester->getFactionTemplateEntry();
    FactionTemplateEntry const*target_faction = target->getFactionTemplateEntry();
    if(!tester_faction || !target_faction)
        return false;

    if(target->isAttackingPlayer() && tester->IsContestedGuard())
        return false;

    // PvC forced reaction and reputation case
    if(tester->GetTypeId()==TYPEID_PLAYER)
    {
        // forced reaction
        if(target_faction->faction)
        {
            if(ReputationRank const* force =((Player*)tester)->GetReputationMgr().GetForcedRankIfAny(target_faction))
                return *force >= REP_FRIENDLY;

            // if faction have reputation then friendly state for tester at 100% dependent from at_war state
            if(FactionEntry const* raw_target_faction = sFactionStore.LookupEntry(target_faction->faction))
                if(FactionState const* factionState = ((Player*)tester)->GetReputationMgr().GetState(raw_target_faction))
                    return !(factionState->Flags & FACTION_FLAG_AT_WAR);
        }
    }
    // CvP forced reaction and reputation case
    else if(target->GetTypeId()==TYPEID_PLAYER)
    {
        // forced reaction
        if(tester_faction->faction)
        {
            if(ReputationRank const* force =((Player*)target)->GetReputationMgr().GetForcedRankIfAny(tester_faction))
                return *force >= REP_FRIENDLY;

            // apply reputation state
            if(FactionEntry const* raw_tester_faction = sFactionStore.LookupEntry(tester_faction->faction))
                if(raw_tester_faction->reputationListID >=0 )
                    return ((Player const*)target)->GetReputationMgr().GetRank(raw_tester_faction) >= REP_FRIENDLY;
        }
    }

    // common faction based case (CvC,PvC,CvP)
    return tester_faction->IsFriendlyTo(*target_faction);
}

bool Unit::IsHostileToPlayers() const
{
    FactionTemplateEntry const* my_faction = getFactionTemplateEntry();
    if(!my_faction || !my_faction->faction)
        return false;

    FactionEntry const* raw_faction = sFactionStore.LookupEntry(my_faction->faction);
    if(raw_faction && raw_faction->reputationListID >=0 )
        return false;

    return my_faction->IsHostileToPlayers();
}

bool Unit::IsNeutralToAll() const
{
    FactionTemplateEntry const* my_faction = getFactionTemplateEntry();
    if(!my_faction || !my_faction->faction)
        return true;

    FactionEntry const* raw_faction = sFactionStore.LookupEntry(my_faction->faction);
    if(raw_faction && raw_faction->reputationListID >=0 )
        return false;

    return my_faction->IsNeutralToAll();
}

bool Unit::Attack(Unit *victim, bool meleeAttack)
{
    if(!victim || victim == this)
        return false;

    // dead units can neither attack nor be attacked
    if(!isAlive() || !victim->IsInWorld() || !victim->isAlive())
        return false;

    // player cannot attack while mounted or in vehicle
    if(GetTypeId()==TYPEID_PLAYER && (IsMounted() || GetVehicle()))
        return false;

    // nobody can attack GM in GM-mode
    if(victim->GetTypeId()==TYPEID_PLAYER)
    {
        if(((Player*)victim)->isGameMaster())
            return false;
    }
    else
    {
        if(((Creature*)victim)->IsInEvadeMode())
            return false;
    }

    // remove SPELL_AURA_MOD_UNATTACKABLE at attack (in case non-interruptible spells stun aura applied also that not let attack)
    if(HasAuraType(SPELL_AURA_MOD_UNATTACKABLE))
        RemoveSpellsCausingAura(SPELL_AURA_MOD_UNATTACKABLE);

    // in fighting already
    if (m_attacking)
    {
        if (m_attacking == victim)
        {
            // switch to melee attack from ranged/magic
            if( meleeAttack && !hasUnitState(UNIT_STAT_MELEE_ATTACKING) )
            {
                addUnitState(UNIT_STAT_MELEE_ATTACKING);
                SendMeleeAttackStart(victim);
                return true;
            }
            return false;
        }

        // remove old target data
        AttackStop(true);
    }
    // new battle
    else
    {
        // set position before any AI calls/assistance
        if(GetTypeId()==TYPEID_UNIT)
            ((Creature*)this)->SetCombatStartPosition(GetPositionX(), GetPositionY(), GetPositionZ());
    }

    // Set our target
    SetTargetGuid(victim->GetObjectGuid());

    if(meleeAttack)
        addUnitState(UNIT_STAT_MELEE_ATTACKING);

    m_attacking = victim;
    m_attacking->_addAttacker(this);

    if (GetTypeId() == TYPEID_UNIT)
    {
        ((Creature*)this)->SendAIReaction(AI_REACTION_HOSTILE);
        ((Creature*)this)->CallAssistance();
    }

    // delay offhand weapon attack to next attack time
    if(haveOffhandWeapon())
        resetAttackTimer(OFF_ATTACK);

    if(meleeAttack)
        SendMeleeAttackStart(victim);

    return true;
}

bool Unit::AttackStop(bool targetSwitch /*=false*/)
{
    if (!m_attacking)
        return false;

    Unit* victim = m_attacking;

    m_attacking->_removeAttacker(this);
    m_attacking = NULL;

    // Clear our target
    SetTargetGuid(ObjectGuid());

    clearUnitState(UNIT_STAT_MELEE_ATTACKING);

    InterruptSpell(CURRENT_MELEE_SPELL);

    // reset only at real combat stop
    if(!targetSwitch && GetTypeId()==TYPEID_UNIT )
    {
        ((Creature*)this)->SetNoCallAssistance(false);

        if (((Creature*)this)->HasSearchedAssistance())
        {
            ((Creature*)this)->SetNoSearchAssistance(false);
            UpdateSpeed(MOVE_RUN, false);
        }
    }

    SendMeleeAttackStop(victim);

    return true;
}

void Unit::CombatStop(bool includingCast)
{
    if (includingCast && IsNonMeleeSpellCasted(false))
        InterruptNonMeleeSpells(false);

    AttackStop();
    RemoveAllAttackers();
    if( GetTypeId()==TYPEID_PLAYER )
        ((Player*)this)->SendAttackSwingCancelAttack();     // melee and ranged forced attack cancel
    ClearInCombat();
}

struct CombatStopWithPetsHelper
{
    explicit CombatStopWithPetsHelper(bool _includingCast) : includingCast(_includingCast) {}
    void operator()(Unit* unit) const { unit->CombatStop(includingCast); }
    bool includingCast;
};

void Unit::CombatStopWithPets(bool includingCast)
{
    CombatStop(includingCast);
    CallForAllControlledUnits(CombatStopWithPetsHelper(includingCast), CONTROLLED_PET|CONTROLLED_GUARDIANS|CONTROLLED_CHARM);
}

struct IsAttackingPlayerHelper
{
    explicit IsAttackingPlayerHelper() {}
    bool operator()(Unit const* unit) const { return unit->isAttackingPlayer(); }
};

bool Unit::isAttackingPlayer() const
{
    if(hasUnitState(UNIT_STAT_ATTACK_PLAYER))
        return true;

    return CheckAllControlledUnits(IsAttackingPlayerHelper(), CONTROLLED_PET|CONTROLLED_TOTEMS|CONTROLLED_GUARDIANS|CONTROLLED_CHARM);
}

void Unit::RemoveAllAttackers()
{
    while (!m_attackers.empty())
    {
        AttackerSet::iterator iter = m_attackers.begin();
        if(!(*iter)->AttackStop())
        {
            sLog.outError("WORLD: Unit has an attacker that isn't attacking it!");
            m_attackers.erase(iter);
        }
    }
}

bool Unit::HasAuraStateForCaster(AuraState flag, uint64 caster) const
{
    if(!HasAuraState(flag))
        return false;

    // single per-caster aura state
    if(flag == AURA_STATE_CONFLAGRATE)
    {
        Unit::AuraList const& dotList = GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
        for(Unit::AuraList::const_iterator i = dotList.begin(); i != dotList.end(); ++i)
        {
            if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK &&
                (*i)->GetCasterGUID() == caster &&
                //  Immolate
                (((*i)->GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000004)) ||
                // Shadowflame
                ((*i)->GetSpellProto()->SpellFamilyFlags2 & 0x00000002)))
            {
                return true;
            }
        }

        return false;
    }

    return true;
}

void Unit::ModifyAuraState(AuraState flag, bool apply)
{
    if (apply)
    {
        if (!HasFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1)))
        {
            SetFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));
            if(GetTypeId() == TYPEID_PLAYER)
            {
                const PlayerSpellMap& sp_list = ((Player*)this)->GetSpellMap();
                for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
                {
                    if(itr->second.state == PLAYERSPELL_REMOVED) continue;
                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                    if (!spellInfo || !IsPassiveSpell(spellInfo)) continue;
                    if (AuraState(spellInfo->CasterAuraState) == flag)
                        CastSpell(this, itr->first, true, NULL);
                }
            }
        }
    }
    else
    {
        if (HasFlag(UNIT_FIELD_AURASTATE,1<<(flag-1)))
        {
            RemoveFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));

            if (flag != AURA_STATE_ENRAGE)                  // enrage aura state triggering continues auras
            {
                Unit::SpellAuraHolderMap& tAuras = GetSpellAuraHolderMap();
                for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
                {
                    SpellEntry const* spellProto = (*itr).second->GetSpellProto();
                    if (AuraState(spellProto->CasterAuraState) == flag)
                    {
                        RemoveSpellAuraHolder(itr->second);
                        itr = tAuras.begin();
                    }
                    else
                        ++itr;
                }
            }
        }
    }
}

Unit *Unit::GetOwner() const
{
    ObjectGuid ownerid = GetOwnerGuid();
    if (!ownerid.IsEmpty())
        return ObjectAccessor::GetUnit(*this, ownerid);
    return NULL;
}

Unit *Unit::GetCharmer() const
{
    ObjectGuid charmerid = GetCharmerGuid();
    if (!charmerid.IsEmpty())
        return ObjectAccessor::GetUnit(*this, charmerid);
    return NULL;
}

Unit *Unit::GetCreator() const
{
    ObjectGuid creatorid = GetCreatorGuid();
    if(!creatorid.IsEmpty())
        return ObjectAccessor::GetUnit(*this, creatorid);
    return NULL;
}

bool Unit::IsCharmerOrOwnerPlayerOrPlayerItself() const
{
    if (GetTypeId()==TYPEID_PLAYER)
        return true;

    return GetCharmerOrOwnerGuid().IsPlayer();
}

Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself()
{
    ObjectGuid guid = GetCharmerOrOwnerGuid();
    if (guid.IsPlayer())
        return ObjectAccessor::FindPlayer(guid);

    return GetTypeId()==TYPEID_PLAYER ? (Player*)this : NULL;
}

Pet* Unit::GetPet() const
{
    ObjectGuid pet_guid = GetPetGuid();
    if (!pet_guid.IsEmpty())
    {
        if (IsInWorld())
        {
            if (Pet* pet = GetMap()->GetPet(pet_guid))
                return pet;
        }

        sLog.outError("Unit::GetPet: %s not exist.", pet_guid.GetString().c_str());
        const_cast<Unit*>(this)->SetPet(0);
    }

    return NULL;
}

Pet* Unit::_GetPet(ObjectGuid guid) const
{
    return ObjectAccessor::FindPet(guid);
}

void Unit::RemoveMiniPet()
{
    if (Pet* pet = GetMiniPet())
        pet->Unsummon(PET_SAVE_AS_DELETED,this);
    else
        SetCritterGuid(ObjectGuid());
}

Pet* Unit::GetMiniPet() const
{
    if (GetCritterGuid().IsEmpty())
        return NULL;

    return GetMap()->GetPet(GetCritterGuid());
}

Unit* Unit::GetCharm() const
{
    ObjectGuid charm_guid = GetCharmGuid();
    if (!charm_guid.IsEmpty())
    {
        if (Unit* pet = ObjectAccessor::GetUnit(*this, charm_guid))
            return pet;

        sLog.outError("Unit::GetCharm: Charmed %s not exist.", charm_guid.GetString().c_str());
        const_cast<Unit*>(this)->SetCharm(NULL);
    }

    return NULL;
}

void Unit::Uncharm()
{
    if (Unit* charm = GetCharm())
    {
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM);
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS);
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS_PET);
        charm->SetCharmerGuid(ObjectGuid());
    }
}

float Unit::GetCombatDistance( const Unit* target ) const
{
    float radius = target->GetFloatValue(UNIT_FIELD_COMBATREACH) + GetFloatValue(UNIT_FIELD_COMBATREACH);
    float dx = GetPositionX() - target->GetPositionX();
    float dy = GetPositionY() - target->GetPositionY();
    float dz = GetPositionZ() - target->GetPositionZ();
    float dist = sqrt((dx*dx) + (dy*dy) + (dz*dz)) - radius;
    return ( dist > 0 ? dist : 0);
}

void Unit::SetPet(Pet* pet)
{
    if (pet)
    {
        SetPetGuid(pet->GetObjectGuid()) ;  //Using last pet guid for player

        AddPetToList(pet);

        if(GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)this)->AddKnownPetName(pet->GetCharmInfo()->GetPetNumber(),pet->GetName());
            ((Player*)this)->SendPetGUIDs();
        }
    }
    else
        SetPetGuid(ObjectGuid());
}

void Unit::SetCharm(Unit* pet)
{
    SetCharmGuid(pet ? pet->GetObjectGuid() : ObjectGuid());
}

void Unit::AddPetToList(Pet* pet)
{
    if (pet)
        m_groupPets.insert(pet->GetObjectGuid());
}

void Unit::RemovePetFromList(Pet* pet)
{
    m_groupPets.erase(pet->GetObjectGuid());

    GroupPetList m_groupPetsTmp = GetPets();
    for(GroupPetList::const_iterator itr = m_groupPetsTmp.begin(); itr != m_groupPetsTmp.end(); ++itr)
    {
        Pet* _pet = GetMap()->GetPet(*itr);
        if (!_pet)
            m_groupPets.erase(*itr);
    }
}

void Unit::AddGuardian( Pet* pet )
{
    m_guardianPets.insert(pet->GetGUID());
}

void Unit::RemoveGuardian( Pet* pet )
{
    if(GetTypeId() == TYPEID_PLAYER)
    {
        uint32 SpellID = pet->GetCreateSpellID();
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(SpellID);
        if (spellInfo && spellInfo->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
        {
            ((Player*)this)->SendCooldownEvent(spellInfo);
        }
    }
    m_guardianPets.erase(pet->GetGUID());
}

void Unit::RemoveGuardians()
{
    while(!m_guardianPets.empty())
    {
        uint64 guid = *m_guardianPets.begin();
        if(Pet* pet = GetMap()->GetPet(guid))
            pet->Unsummon(PET_SAVE_AS_DELETED, this);

        m_guardianPets.erase(guid);
    }
    m_guardianPets.clear();
}

Pet* Unit::FindGuardianWithEntry(uint32 entry)
{
    for(GuardianPetList::const_iterator itr = m_guardianPets.begin(); itr != m_guardianPets.end(); ++itr)
        if(Pet* pet = GetMap()->GetPet(*itr))
            if (pet->GetEntry() == entry)
                return pet;

    return NULL;
}

Pet* Unit::GetProtectorPet()
{
    for(GuardianPetList::const_iterator itr = m_guardianPets.begin(); itr != m_guardianPets.end(); ++itr)
        if(Pet* pet = GetMap()->GetPet(*itr))
            if (pet->getPetType() == PROTECTOR_PET)
                return pet;

    return NULL;
}

Unit* Unit::_GetTotem(TotemSlot slot) const
{
    return GetTotem(slot);
}

Totem* Unit::GetTotem(TotemSlot slot ) const
{
    if(slot >= MAX_TOTEM_SLOT || !IsInWorld())
        return NULL;

    Creature *totem = GetMap()->GetCreature(m_TotemSlot[slot]);
    return totem && totem->IsTotem() ? (Totem*)totem : NULL;
}

bool Unit::IsAllTotemSlotsUsed() const
{
    for (int i = 0; i < MAX_TOTEM_SLOT; ++i)
        if (!m_TotemSlot[i])
            return false;
    return true;
}

void Unit::_AddTotem(TotemSlot slot, Totem* totem)
{
    m_TotemSlot[slot] = totem->GetGUID();
}

void Unit::_RemoveTotem(Totem* totem)
{
    for(int i = 0; i < MAX_TOTEM_SLOT; ++i)
    {
        if (m_TotemSlot[i] == totem->GetGUID())
        {
            m_TotemSlot[i] = 0;
            break;
        }
    }
}

void Unit::UnsummonAllTotems()
{
    for (int i = 0; i < MAX_TOTEM_SLOT; ++i)
        if (Totem* totem = GetTotem(TotemSlot(i)))
            totem->UnSummon();
}

int32 Unit::DealHeal(Unit *pVictim, uint32 addhealth, SpellEntry const *spellProto, bool critical, uint32 absorb)
{
    int32 gain = pVictim->ModifyHealth(int32(addhealth));

    Unit* unit = this;

    if( GetTypeId()==TYPEID_UNIT && ((Creature*)this)->IsTotem() && ((Totem*)this)->GetTotemType()!=TOTEM_STATUE)
        unit = GetOwner();

    if (unit->GetTypeId()==TYPEID_PLAYER)
    {
        // overheal = addhealth - gain
        unit->SendHealSpellLog(pVictim, spellProto->Id, addhealth, addhealth - gain, critical, absorb);

        if (BattleGround *bg = ((Player*)unit)->GetBattleGround())
            bg->UpdatePlayerScore((Player*)unit, SCORE_HEALING_DONE, gain);

        // use the actual gain, as the overheal shall not be counted, skip gain 0 (it ignored anyway in to criteria)
        if (gain)
            ((Player*)unit)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE, gain, 0, pVictim);

        ((Player*)unit)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED, addhealth);
    }

    if (pVictim->GetTypeId()==TYPEID_PLAYER)
    {
        ((Player*)pVictim)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED, gain);
        ((Player*)pVictim)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED, addhealth);
    }

    return gain;
}

Unit* Unit::SelectMagnetTarget(Unit *victim, Spell* spell, SpellEffectIndex eff)
{
    if(!victim)
        return NULL;

    // Magic case
    if (spell && (spell->m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_NONE || spell->m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC))
    {
        Unit::AuraList const& magnetAuras = victim->GetAurasByType(SPELL_AURA_SPELL_MAGNET);
        for(Unit::AuraList::const_iterator itr = magnetAuras.begin(); itr != magnetAuras.end(); ++itr)
        {
            if (Unit* magnet = (*itr)->GetCaster())
            {
                if (magnet->isAlive() && magnet->IsWithinLOSInMap(this) && spell->CheckTarget(magnet, eff))
                {
                    if (SpellAuraHolder *holder = (*itr)->GetHolder())
                        if (holder->DropAuraCharge())
                            victim->RemoveSpellAuraHolder(holder);
                    return magnet;
                }
            }
        }
    }
    // Melee && ranged case
    else
    {
        AuraList const& hitTriggerAuras = victim->GetAurasByType(SPELL_AURA_ADD_CASTER_HIT_TRIGGER);
        for(AuraList::const_iterator i = hitTriggerAuras.begin(); i != hitTriggerAuras.end(); ++i)
        {
            if (Unit* magnet = (*i)->GetCaster())
            {
                if (magnet->isAlive() && magnet->IsWithinLOSInMap(this) && (!spell || spell->CheckTarget(magnet, eff)))
                {
                    if (roll_chance_i((*i)->GetModifier()->m_amount))
                    {
                        if (SpellAuraHolder *holder = (*i)->GetHolder())
                            if (holder->DropAuraCharge())
                                victim->RemoveSpellAuraHolder(holder);
                        return magnet;
                    }
                }
            }
        }
    }

    return victim;
}

void Unit::SendHealSpellLog(Unit *pVictim, uint32 SpellID, uint32 Damage, uint32 OverHeal, bool critical, uint32 absorb)
{
    // we guess size
    WorldPacket data(SMSG_SPELLHEALLOG, (8+8+4+4+1));
    data << pVictim->GetPackGUID();
    data << GetPackGUID();
    data << uint32(SpellID);
    data << uint32(Damage);
    data << uint32(OverHeal);
    data << uint32(absorb);
    data << uint8(critical ? 1 : 0);
    data << uint8(0);                                       // unused in client?
    SendMessageToSet(&data, true);
}

void Unit::SendEnergizeSpellLog(Unit *pVictim, uint32 SpellID, uint32 Damage, Powers powertype)
{
    WorldPacket data(SMSG_SPELLENERGIZELOG, (8+8+4+4+4+1));
    data << pVictim->GetPackGUID();
    data << GetPackGUID();
    data << uint32(SpellID);
    data << uint32(powertype);
    data << uint32(Damage);
    SendMessageToSet(&data, true);
}

void Unit::EnergizeBySpell(Unit *pVictim, uint32 SpellID, uint32 Damage, Powers powertype)
{
    SendEnergizeSpellLog(pVictim, SpellID, Damage, powertype);
    // needs to be called after sending spell log
    pVictim->ModifyPower(powertype, Damage);
}

int32 Unit::SpellBonusWithCoeffs(SpellEntry const *spellProto, int32 total, int32 benefit, int32 ap_benefit,  DamageEffectType damagetype, bool donePart, float defCoeffMod)
{
    // Distribute Damage over multiple effects, reduce by AoE
    float coeff;

    // Not apply this to creature casted spells
    if (GetTypeId()==TYPEID_UNIT && !((Creature*)this)->IsPet())
        coeff = 1.0f;
    // Check for table values
    else if (SpellBonusEntry const* bonus = sSpellMgr.GetSpellBonusData(spellProto->Id))
    {
        coeff = damagetype == DOT ? bonus->dot_damage : bonus->direct_damage;

        // apply ap bonus at done part calculation only (it flat total mod so common with taken)
        if (donePart && (bonus->ap_bonus || bonus->ap_dot_bonus))
        {
            float ap_bonus = damagetype == DOT ? bonus->ap_dot_bonus : bonus->ap_bonus;

            // Impurity
            if (GetTypeId() == TYPEID_PLAYER && spellProto->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT)
            {
                if (SpellEntry const* spell = ((Player*)this)->GetKnownTalentRankById(2005))
                    ap_bonus += ((spell->CalculateSimpleValue(EFFECT_INDEX_0) * ap_bonus) / 100.0f);
            }

            total += int32(ap_bonus * (GetTotalAttackPowerValue(IsSpellRequiresRangedAP(spellProto) ? RANGED_ATTACK : BASE_ATTACK) + ap_benefit));
        }
    }
    // Default calculation
    else if (benefit)
        coeff = CalculateDefaultCoefficient(spellProto, damagetype) * defCoeffMod;

    if (benefit)
    {
        float LvlPenalty = CalculateLevelPenalty(spellProto);

        // Spellmod SpellDamage
        if(Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_SPELL_BONUS_DAMAGE, coeff);
            coeff /= 100.0f;
        }

        total += int32(benefit * coeff * LvlPenalty);
    }

    return total;
};

/**
 * Calculates caster part of spell damage bonuses,
 * also includes different bonuses dependent from target auras
 */
uint32 Unit::SpellDamageBonusDone(Unit *pVictim, SpellEntry const *spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack)
{
    if(!spellProto || !pVictim || damagetype==DIRECT_DAMAGE || spellProto->AttributesEx6 & SPELL_ATTR_EX6_NO_DMG_MODS)
        return pdamage;

    // For totems get damage bonus from owner (statue isn't totem in fact)
    if( GetTypeId()==TYPEID_UNIT && ((Creature*)this)->IsTotem() && ((Totem*)this)->GetTotemType()!=TOTEM_STATUE)
    {
        if(Unit* owner = GetOwner())
            return owner->SpellDamageBonusDone(pVictim, spellProto, pdamage, damagetype);
    }

    float DoneTotalMod = 1.0f;
    int32 DoneTotal = 0;

    // Creature damage
    if( GetTypeId() == TYPEID_UNIT && !((Creature*)this)->IsPet() )
        DoneTotalMod *= ((Creature*)this)->GetSpellDamageMod(((Creature*)this)->GetCreatureInfo()->rank);

    AuraList const& mModDamagePercentDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    for(AuraList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
    {
        if( ((*i)->GetModifier()->m_miscvalue & GetSpellSchoolMask(spellProto)) &&
            (*i)->GetSpellProto()->EquippedItemClass == -1 &&
                                                            // -1 == any item class (not wand then)
            (*i)->GetSpellProto()->EquippedItemInventoryTypeMask == 0 )
                                                            // 0 == any inventory type (not wand then)
        {
            // bonus stored in another auras basepoints
            if ((*i)->GetModifier()->m_amount == 0)
            {
                // Clearcasting - bonus from Elemental Oath
                if ((*i)->GetSpellProto()->Id == 16246)
                {
                    AuraList const& aurasCrit = GetAurasByType(SPELL_AURA_MOD_SPELL_CRIT_CHANCE);
                    for (AuraList::const_iterator itr = aurasCrit.begin(); itr != aurasCrit.end(); itr++)
                    {
                        if ((*itr)->GetSpellProto()->SpellIconID == 3053)
                        {
                            DoneTotalMod *= ((*itr)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1) + 100.0f) / 100.0f;
                            break;
                        }
                    }
                }
            }
            else
                DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
        }
    }

    uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
    // Add flat bonus from spell damage versus
    DoneTotal += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS, creatureTypeMask);
    AuraList const& mDamageDoneVersus = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
    for(AuraList::const_iterator i = mDamageDoneVersus.begin();i != mDamageDoneVersus.end(); ++i)
        if(creatureTypeMask & uint32((*i)->GetModifier()->m_miscvalue))
            DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;

    AuraList const& mDamageDoneCreature = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE);
    for(AuraList::const_iterator i = mDamageDoneCreature.begin();i != mDamageDoneCreature.end(); ++i)
    {
        if(creatureTypeMask & uint32((*i)->GetModifier()->m_miscvalue))
            DoneTotalMod += ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
    }

    // done scripted mod (take it from owner)
    Unit *owner = GetOwner();
    if (!owner) owner = this;
    AuraList const& mOverrideClassScript= owner->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for(AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->isAffectedOnSpell(spellProto))
            continue;
        switch((*i)->GetModifier()->m_miscvalue)
        {
            case 4920: // Molten Fury
            case 4919:
            case 6917: // Death's Embrace
            case 6926:
            case 6928:
            {
                if(pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                    DoneTotalMod *= (100.0f+(*i)->GetModifier()->m_amount)/100.0f;
                break;
            }
            // Soul Siphon
            case 4992:
            case 4993:
            {
                // effect 1 m_amount
                int32 maxPercent = (*i)->GetModifier()->m_amount;
                // effect 0 m_amount
                int32 stepPercent = CalculateSpellDamage(this, (*i)->GetSpellProto(), EFFECT_INDEX_0);
                // count affliction effects and calc additional damage in percentage
                int32 modPercent = 0;
                SpellAuraHolderMap const& victimAuras = pVictim->GetSpellAuraHolderMap();
                for (SpellAuraHolderMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                {
                    SpellEntry const* m_spell = itr->second->GetSpellProto();
                    if (m_spell->SpellFamilyName != SPELLFAMILY_WARLOCK || !(m_spell->SpellFamilyFlags & UI64LIT(0x0004071B8044C402)))
                        continue;
                    modPercent += stepPercent * itr->second->GetStackAmount();
                    if (modPercent >= maxPercent)
                    {
                        modPercent = maxPercent;
                        break;
                    }
                }
                DoneTotalMod *= (modPercent+100.0f)/100.0f;
                break;
            }
            case 6916: // Death's Embrace
            case 6925:
            case 6927:
                if (HasAuraState(AURA_STATE_HEALTHLESS_20_PERCENT))
                    DoneTotalMod *= (100.0f+(*i)->GetModifier()->m_amount)/100.0f;
                break;
            case 5481: // Starfire Bonus
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, UI64LIT(0x0000000000200002)))
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                break;
            }
            case 4418: // Increased Shock Damage
            case 4554: // Increased Lightning Damage
            case 4555: // Improved Moonfire
            case 5142: // Increased Lightning Damage
            case 5147: // Improved Consecration / Libram of Resurgence
            case 5148: // Idol of the Shooting Star
            case 6008: // Increased Lightning Damage
            case 8627: // Totem of Hex
            {
                DoneTotal+=(*i)->GetModifier()->m_amount;
                break;
            }
            // Tundra Stalker
            // Merciless Combat
            case 7277:
            {
                // Merciless Combat
                if ((*i)->GetSpellProto()->SpellIconID == 2656)
                {
                    if(pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                        DoneTotalMod *= (100.0f+(*i)->GetModifier()->m_amount)/100.0f;
                }
                else // Tundra Stalker
                {
                    // Frost Fever (target debuff)
                    if (pVictim->GetAura(SPELL_AURA_MOD_MELEE_HASTE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0000000000000000), 0x00000002))
                        DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                    break;
                }
                break;
            }
            case 7293: // Rage of Rivendare
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0200000000000000)))
                    DoneTotalMod *= ((*i)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1)*2+100.0f)/100.0f;
                break;
            }
            // Twisted Faith
            case 7377:
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, UI64LIT(0x0000000000008000), 0, GetGUID()))
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                break;
            }
            // Marked for Death
            case 7598:
            case 7599:
            case 7600:
            case 7601:
            case 7602:
            {
                if (pVictim->GetAura(SPELL_AURA_MOD_STALKED, SPELLFAMILY_HUNTER, UI64LIT(0x0000000000000400)))
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                break;
            }
        }
    }

    // custom scripted mod from dummy
    AuraList const& mDummy = owner->GetAurasByType(SPELL_AURA_DUMMY);
    for(AuraList::const_iterator i = mDummy.begin(); i != mDummy.end(); ++i)
    {
        SpellEntry const *spell = (*i)->GetSpellProto();
        //Fire and Brimstone
        if (spell->SpellFamilyName == SPELLFAMILY_WARLOCK && spell->SpellIconID == 3173)
        {
            if (pVictim->HasAuraState(AURA_STATE_CONFLAGRATE) && (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && spellProto->SpellFamilyFlags & UI64LIT(0x0002004000000000)))
            {
                DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f) / 100.0f;
                break;
            }
        }
    }

     // Custom scripted damage
    switch(spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_MAGE:
        {
            // Ice Lance
            if (spellProto->SpellIconID == 186)
            {
                if (pVictim->isFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                {
                    float multiplier = 3.0f;

                    // if target have higher level
                    if (pVictim->getLevel() > getLevel())
                        // Glyph of Ice Lance
                        if (Aura* glyph = GetDummyAura(56377))
                            multiplier = glyph->GetModifier()->m_amount;

                    DoneTotalMod *= multiplier;
                }
            }
            // Torment the weak affected (Arcane Barrage, Arcane Blast, Frostfire Bolt, Arcane Missiles, Fireball)
            if ((spellProto->SpellFamilyFlags & UI64LIT(0x0000900020200021)) &&
                (pVictim->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED) || pVictim->HasAuraType(SPELL_AURA_HASTE_ALL)))
            {
                //Search for Torment the weak dummy aura
                Unit::AuraList const& ttw = GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = ttw.begin(); i != ttw.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 3263)
                    {
                        DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f) / 100.0f;
                        break;
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Drain Soul
            if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000004000))
            {
                if (pVictim->GetHealth() * 100 / pVictim->GetMaxHealth() <= 25)
                    DoneTotalMod *= 4;
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Glyph of Smite
            if (spellProto->SpellFamilyFlags & UI64LIT(0x00000080))
            {
                // Holy Fire
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, UI64LIT(0x00100000)))
                    if (Aura *aur = GetAura(55692, EFFECT_INDEX_0))
                        DoneTotalMod *= (aur->GetModifier()->m_amount+100.0f) / 100.0f;
            }
            // Mind Flay
            else if (spellProto->SpellFamilyFlags & UI64LIT(0x00800000))
            {
                // Shadow Word: Pain
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, UI64LIT(0x00008000)))
                {
                    // Glyph of Mind Flay
                    if (Aura *aur = GetAura(55687, EFFECT_INDEX_0))
                        DoneTotalMod *= (aur->GetModifier()->m_amount+100.0f) / 100.0f;
                    // Twisted Faith
                    Unit::AuraList const& tf = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                    for(Unit::AuraList::const_iterator i = tf.begin(); i != tf.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->SpellIconID == 2848 && (*i)->GetEffIndex() == 1)
                        {
                            DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f) / 100.0f;
                            break;
                        }
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Improved Insect Swarm (Wrath part)
            if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001))
            {
                // if Insect Swarm on target
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, UI64LIT(0x000000000200000), 0, GetGUID()))
                {
                    Unit::AuraList const& improvedSwarm = GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator iter = improvedSwarm.begin(); iter != improvedSwarm.end(); ++iter)
                    {
                        if ((*iter)->GetSpellProto()->SpellIconID == 1771)
                        {
                            DoneTotalMod *= ((*iter)->GetModifier()->m_amount+100.0f) / 100.0f;
                            break;
                        }
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Icy Touch and Howling Blast
            if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000200000002))
            {
                // search disease
                bool found = false;
                Unit::SpellAuraHolderMap const& auras = pVictim->GetSpellAuraHolderMap();
                for(Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr!=auras.end(); ++itr)
                {
                    if(itr->second->GetSpellProto()->Dispel == DISPEL_DISEASE)
                    {
                        found = true;
                        break;
                    }
                }

                // search for Glacier Rot and  Improved Icy Touch dummy aura
                bool isIcyTouch=(spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000002));
                Unit::AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
                {
                    if ((found && (*i)->GetSpellProto()->EffectMiscValue[(*i)->GetEffIndex()] == 7244) || //Glacier Rot
                        (isIcyTouch && (*i)->GetSpellProto()->SpellIconID == 2721))                       //Improved Icy Touch 
                    {
                        DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f) / 100.0f;
                    }
                }

                // Icy Touch
                if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000002))
                {
                    // Improved Icy Touch
                    Unit::AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->SpellIconID == 2721)
                        {
                            DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f) / 100.0f;
                            break;
                        }
                    }
                }
            }
            // Death Coil (bonus from Item - Death Knight T8 DPS Relic)
            else if (spellProto->SpellFamilyFlags & UI64LIT(0x00002000))
            {
                 if (Aura* sigil = GetDummyAura(64962))
                    DoneTotal += sigil->GetModifier()->m_amount;
            }
            break;
        }
        default:
            break;
    }

    // Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit = SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto));

    // apply ap bonus and benefit affected by spell power implicit coeffs and spell level penalties
    DoneTotal = SpellBonusWithCoeffs(spellProto, DoneTotal, DoneAdvertisedBenefit, 0, damagetype, true);

    float tmpDamage = (int32(pdamage) + DoneTotal * int32(stack)) * DoneTotalMod;
    // apply spellmod to Done damage (flat and pct)
    if(Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, tmpDamage);

    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}

/**
 * Calculates target part of spell damage bonuses,
 * will be called on each tick for periodic damage over time auras
 */
uint32 Unit::SpellDamageBonusTaken(Unit *pCaster, SpellEntry const *spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack)
{
    if(!spellProto || !pCaster || damagetype==DIRECT_DAMAGE )
        return pdamage;

    uint32 schoolMask = spellProto->SchoolMask;

    // Taken total percent damage auras
    float TakenTotalMod = 1.0f;
    int32 TakenTotal = 0;

    // ..taken
    TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, schoolMask);

    // .. taken pct: dummy auras
    // .. taken (dummy auras)
    AuraList const& m_dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for(AuraList::const_iterator itr = m_dummyAuras.begin(); itr != m_dummyAuras.end(); ++itr)
    {
        switch((*itr)->GetSpellProto()->SpellIconID)
        {
            // Cheat Death
            case 2109:
            {
                if(GetTypeId() != TYPEID_PLAYER)
                    continue;

                float mod = -((Player*)this)->GetRatingBonusValue(CR_CRIT_TAKEN_SPELL)*2*4;
                if (mod < float((*itr)->GetModifier()->m_amount))
                    mod = float((*itr)->GetModifier()->m_amount);
                TakenTotalMod *= (mod+100.0f)/100.0f;
                break;
            }
            // Ebon Plague
            case 1933:
            {
                if ((*itr)->GetMiscValue() & (spellProto ? GetSpellSchoolMask(spellProto) : 0))
                {
                    if (spellProto && spellProto->Dispel == DISPEL_DISEASE)
                        TakenTotalMod *= ((*itr)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_0) + 100.0f) / 100.0f;
                    else
                        TakenTotalMod *= ((*itr)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            }
            default:
                break;
        }
    }

    // From caster spells
    AuraList const& mOwnerTaken = GetAurasByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
    for(AuraList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
    {
        if ((*i)->GetCasterGuid() == pCaster->GetObjectGuid() && (*i)->isAffectedOnSpell(spellProto))
            TakenTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
    }

    // Mod damage from spell mechanic
    TakenTotalMod *= GetTotalAuraMultiplierByMiscValueForMask(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT,GetAllSpellMechanicMask(spellProto));

    // Mod damage taken from AoE spells
    if(IsAreaOfEffectSpell(spellProto))
    {
        TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE, schoolMask);
        if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
            TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_PET_AOE_DAMAGE_AVOIDANCE, schoolMask);
    }

    // Taken fixed damage bonus auras
    int32 TakenAdvertisedBenefit = SpellBaseDamageBonusTaken(GetSpellSchoolMask(spellProto));

    // apply benefit affected by spell power implicit coeffs and spell level penalties
    TakenTotal = SpellBonusWithCoeffs(spellProto, TakenTotal, TakenAdvertisedBenefit, 0, damagetype, false);

    float tmpDamage = (int32(pdamage) + TakenTotal * int32(stack)) * TakenTotalMod;

    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}

int32 Unit::SpellBaseDamageBonusDone(SpellSchoolMask schoolMask)
{
    int32 DoneAdvertisedBenefit = 0;

    // ..done
    AuraList const& mDamageDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);
    for(AuraList::const_iterator i = mDamageDone.begin();i != mDamageDone.end(); ++i)
    {
        if (!(*i)->GetHolder() || (*i)->GetHolder()->IsDeleted())
            continue;

        if (((*i)->GetModifier()->m_miscvalue & schoolMask) != 0 &&
            (*i)->GetSpellProto()->EquippedItemClass == -1 &&                   // -1 == any item class (not wand then)
            (*i)->GetSpellProto()->EquippedItemInventoryTypeMask == 0)          //  0 == any inventory type (not wand then)
                DoneAdvertisedBenefit += (*i)->GetModifier()->m_amount;
    }

    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        DoneAdvertisedBenefit +=((Player*)this)->GetBaseSpellPowerBonus();

        // Damage bonus from stats
        AuraList const& mDamageDoneOfStatPercent = GetAurasByType(SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT);
        for(AuraList::const_iterator i = mDamageDoneOfStatPercent.begin();i != mDamageDoneOfStatPercent.end(); ++i)
        {
            if((*i)->GetModifier()->m_miscvalue & schoolMask)
            {
                // stat used stored in miscValueB for this aura
                Stats usedStat = Stats((*i)->GetMiscBValue());
                DoneAdvertisedBenefit += int32(GetStat(usedStat) * (*i)->GetModifier()->m_amount / 100.0f);
            }
        }
        // ... and attack power
        AuraList const& mDamageDonebyAP = GetAurasByType(SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER);
        for(AuraList::const_iterator i =mDamageDonebyAP.begin();i != mDamageDonebyAP.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue & schoolMask)
                DoneAdvertisedBenefit += int32(GetTotalAttackPowerValue(BASE_ATTACK) * (*i)->GetModifier()->m_amount / 100.0f);
        }

    }
    return DoneAdvertisedBenefit;
}

int32 Unit::SpellBaseDamageBonusTaken(SpellSchoolMask schoolMask)
{
    int32 TakenAdvertisedBenefit = 0;

    // ..taken
    AuraList const& mDamageTaken = GetAurasByType(SPELL_AURA_MOD_DAMAGE_TAKEN);
    for(AuraList::const_iterator i = mDamageTaken.begin();i != mDamageTaken.end(); ++i)
    {
        if(((*i)->GetModifier()->m_miscvalue & schoolMask) != 0)
            TakenAdvertisedBenefit += (*i)->GetModifier()->m_amount;
    }

    return TakenAdvertisedBenefit;
}

bool Unit::IsSpellCrit(Unit *pVictim, SpellEntry const *spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType)
{
    // mobs can't crit with spells at all
    if (GetObjectGuid().IsCreature())
        return false;

    // not critting spell
    if((spellProto->AttributesEx2 & SPELL_ATTR_EX2_CANT_CRIT))
        return false;

    float crit_chance = 0.0f;
    switch(spellProto->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_NONE:
            if (spellProto->Id != 379 && spellProto->Id != 33778) // Earth Shield and Lifebloom heal should be able to crit
                return false;
        case SPELL_DAMAGE_CLASS_MAGIC:
        {
            if (schoolMask & SPELL_SCHOOL_MASK_NORMAL)
                crit_chance = 0.0f;
            // For other schools
            else if (GetTypeId() == TYPEID_PLAYER)
                crit_chance = GetFloatValue( PLAYER_SPELL_CRIT_PERCENTAGE1 + GetFirstSchoolInMask(schoolMask));
            else
            {
                crit_chance = float(m_baseSpellCritChance);
                crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            }
            // taken
            if (pVictim)
            {
                if (!IsPositiveSpell(spellProto->Id))
                {
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE
                    crit_chance += pVictim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE, schoolMask);
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE
                    crit_chance += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);
                    // Modify by player victim resilience
                    crit_chance -= pVictim->GetSpellCritChanceReduction();
                }

                // scripted (increase crit chance ... against ... target by x%)
                // scripted (Increases the critical effect chance of your .... by x% on targets ...)
                AuraList const& mOverrideClassScript = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for(AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
                {
                    if (!((*i)->isAffectedOnSpell(spellProto)))
                        continue;
                    switch((*i)->GetModifier()->m_miscvalue)
                    {
                        case  849:                          //Shatter Rank 1
                            if (pVictim->isFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                                crit_chance+= 17.0f;
                            break;
                        case  910:                          //Shatter Rank 2
                            if (pVictim->isFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                                crit_chance+= 34.0f;
                            break;
                        case  911:                          //Shatter Rank 3
                            if (pVictim->isFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                                crit_chance+= 50.0f;
                            break;
                        case 7917:                          // Glyph of Shadowburn
                            if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                                crit_chance+=(*i)->GetModifier()->m_amount;
                            break;
                        case 7997:                          // Renewed Hope
                        case 7998:
                            if (pVictim->HasAura(6788))
                                crit_chance+=(*i)->GetModifier()->m_amount;
                            break;
                        default:
                            break;
                    }
                }

                // Custom crit by class
                switch(spellProto->SpellFamilyName)
                {
                    case SPELLFAMILY_PRIEST:
                        // Flash Heal
                        if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000800))
                        {
                            if (pVictim->GetHealth() > pVictim->GetMaxHealth()/2)
                                break;
                            AuraList const& mDummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
                            for(AuraList::const_iterator i = mDummyAuras.begin(); i!= mDummyAuras.end(); ++i)
                            {
                                // Improved Flash Heal
                                if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_PRIEST &&
                                    (*i)->GetSpellProto()->SpellIconID == 2542)
                                {
                                    crit_chance+=(*i)->GetModifier()->m_amount;
                                    break;
                                }
                            }
                        }
                        break;
                    case SPELLFAMILY_DRUID:
                        // Improved Insect Swarm (Starfire part)
                        if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000004))
                        {
                            // search for Moonfire on target
                            if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, UI64LIT(0x000000000000002), 0, GetGUID()))
                            {
                                Unit::AuraList const& improvedSwarm = GetAurasByType(SPELL_AURA_DUMMY);
                                for(Unit::AuraList::const_iterator iter = improvedSwarm.begin(); iter != improvedSwarm.end(); ++iter)
                                {
                                    if ((*iter)->GetSpellProto()->SpellIconID == 1771)
                                    {
                                        crit_chance += (*iter)->GetModifier()->m_amount;
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    case SPELLFAMILY_PALADIN:
                        // Sacred Shield
                        if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000040000000))
                        {
                            Aura *aura = pVictim->GetDummyAura(58597);
                            if (aura && aura->GetCasterGuid() == GetObjectGuid())
                                crit_chance+=aura->GetModifier()->m_amount;
                        }
                        // Exorcism
                        else if (spellProto->Category == 19)
                        {
                            if (pVictim->GetCreatureTypeMask() & CREATURE_TYPEMASK_DEMON_OR_UNDEAD)
                                return true;
                        }
                        break;
                    case SPELLFAMILY_SHAMAN:
                        // Lava Burst
                        if (spellProto->SpellFamilyFlags & UI64LIT(0x0000100000000000))
                        {
                            // Flame Shock
                            if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN, UI64LIT(0x0000000010000000), 0, GetGUID()))
                                return true;
                        }
                        break;
                }
            }
            break;
        }
        case SPELL_DAMAGE_CLASS_MELEE:
            // Rend and Tear crit chance with Ferocious Bite on bleeding target
            if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID)
            {
                if(spellProto->SpellFamilyFlags & UI64LIT(0x0000000000800000))
                {
                    if(pVictim->HasAuraState(AURA_STATE_MECHANIC_BLEED))
                    {
                        Unit::AuraList const& aura = GetAurasByType(SPELL_AURA_DUMMY);
                        for(Unit::AuraList::const_iterator itr = aura.begin(); itr != aura.end(); ++itr)
                        {
                            if ((*itr)->GetSpellProto()->SpellIconID == 2859 && (*itr)->GetEffIndex() == 1)
                            {
                                crit_chance += (*itr)->GetModifier()->m_amount;
                                break;
                            }
                        }
                    }
                }
            }
            // do not use break here
        case SPELL_DAMAGE_CLASS_RANGED:
        {
            if (pVictim)
                crit_chance = GetUnitCriticalChance(attackType, pVictim);

            crit_chance+= GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            break;
        }
        default:
            return false;
    }
    // percent done
    // only players use intelligence for critical chance computations
    if(Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRITICAL_CHANCE, crit_chance);

    crit_chance = crit_chance > 0.0f ? crit_chance : 0.0f;
    if (roll_chance_f(crit_chance))
        return true;
    return false;
}

uint32 Unit::SpellCriticalDamageBonus(SpellEntry const *spellProto, uint32 damage, Unit *pVictim)
{
    // Calculate critical bonus
    int32 crit_bonus;
    switch(spellProto->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_MELEE:                      // for melee based spells is 100%
        case SPELL_DAMAGE_CLASS_RANGED:
            crit_bonus = damage;
            break;
        default:
            crit_bonus = damage / 2;                        // for spells is 50%
            break;
    }

    // adds additional damage to crit_bonus (from talents)
    if(Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRIT_DAMAGE_BONUS, crit_bonus);

    if(!pVictim)
        return damage += crit_bonus;

    int32 critPctDamageMod = 0;
    if(spellProto->DmgClass >= SPELL_DAMAGE_CLASS_MELEE)
    {
        if(GetWeaponAttackType(spellProto) == RANGED_ATTACK)
            critPctDamageMod += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
        else
            critPctDamageMod += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);
    }
    else
        critPctDamageMod += pVictim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_DAMAGE,GetSpellSchoolMask(spellProto));

    critPctDamageMod += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS, GetSpellSchoolMask(spellProto));

    uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
    critPctDamageMod += GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CRIT_PERCENT_VERSUS, creatureTypeMask);

    if(critPctDamageMod!=0)
        crit_bonus = int32(crit_bonus * float((100.0f + critPctDamageMod)/100.0f));

    if(crit_bonus > 0)
        damage += crit_bonus;

    return damage;
}

uint32 Unit::SpellCriticalHealingBonus(SpellEntry const *spellProto, uint32 damage, Unit *pVictim)
{
    // Calculate critical bonus
    int32 crit_bonus;
    switch(spellProto->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_MELEE:                      // for melee based spells is 100%
        case SPELL_DAMAGE_CLASS_RANGED:
            // TODO: write here full calculation for melee/ranged spells
            crit_bonus = damage;
            break;
        default:
            crit_bonus = damage / 2;                        // for spells is 50%
            break;
    }

    if(pVictim)
    {
        uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
        crit_bonus = int32(crit_bonus * GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CRIT_PERCENT_VERSUS, creatureTypeMask));
    }

    if(crit_bonus > 0)
        damage += crit_bonus;

    damage = int32(damage * GetTotalAuraMultiplier(SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT));

    return damage;
}

/**
 * Calculates caster part of healing spell bonuses,
 * also includes different bonuses dependent from target auras
 */
uint32 Unit::SpellHealingBonusDone(Unit *pVictim, SpellEntry const *spellProto, int32 healamount, DamageEffectType damagetype, uint32 stack)
{
     // For totems get healing bonus from owner (statue isn't totem in fact)
    if( GetTypeId()==TYPEID_UNIT && ((Creature*)this)->IsTotem() && ((Totem*)this)->GetTotemType()!=TOTEM_STATUE)
        if(Unit* owner = GetOwner())
            return owner->SpellHealingBonusDone(pVictim, spellProto, healamount, damagetype, stack);

    // No heal amount for this class spells
    if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE)
        return healamount < 0 ? 0 : healamount;

    // Healing Done
    // Done total percent damage auras
    float  DoneTotalMod = 1.0f;
    int32  DoneTotal = 0;

    // Healing done percent
    AuraList const& mHealingDonePct = GetAurasByType(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
    for(AuraList::const_iterator i = mHealingDonePct.begin();i != mHealingDonePct.end(); ++i)
        DoneTotalMod *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;

    // done scripted mod (take it from owner)
    Unit *owner = GetOwner();
    if (!owner) owner = this;
    AuraList const& mOverrideClassScript= owner->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for(AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->isAffectedOnSpell(spellProto))
            continue;
        switch((*i)->GetModifier()->m_miscvalue)
        {
            case 4415: // Increased Rejuvenation Healing
            case 4953:
            case 3736: // Hateful Totem of the Third Wind / Increased Lesser Healing Wave / LK Arena (4/5/6) Totem of the Third Wind / Savage Totem of the Third Wind
                DoneTotal+=(*i)->GetModifier()->m_amount;
                break;
            case 7997: // Renewed Hope
            case 7998:
                if (pVictim->HasAura(6788))
                    DoneTotalMod *=((*i)->GetModifier()->m_amount + 100.0f)/100.0f;
                break;
            case   21: // Test of Faith
            case 6935:
            case 6918:
                if (pVictim->GetHealth() < pVictim->GetMaxHealth()/2)
                    DoneTotalMod *=((*i)->GetModifier()->m_amount + 100.0f)/100.0f;
                break;
            case 7798: // Glyph of Regrowth
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_DRUID, UI64LIT(0x0000000000000040)))
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                break;
            }
            case 8477: // Nourish Heal Boost
            {
                int32 stepPercent = (*i)->GetModifier()->m_amount;

                int ownHotCount = 0;                        // counted HoT types amount, not stacks

                Unit::AuraList const& RejorRegr = pVictim->GetAurasByType(SPELL_AURA_PERIODIC_HEAL);
                for(Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
                    if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID &&
                        (*i)->GetCasterGuid() == GetObjectGuid())
                        ++ownHotCount;

                if (ownHotCount)
                    DoneTotalMod *= (stepPercent * ownHotCount + 100.0f) / 100.0f;
                break;
            }
            case 7871: // Glyph of Lesser Healing Wave
            {
                if (pVictim->GetAura(SPELL_AURA_DUMMY, SPELLFAMILY_SHAMAN, UI64LIT(0x0000040000000000), 0, GetGUID()))
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                break;
            }
            default:
                break;
        }
    }

    // Nourish 20% of heal increase if target is affected by Druids HOTs
    if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && (spellProto->SpellFamilyFlags & UI64LIT(0x0200000000000000)))
    {
        int ownHotCount = 0;                        // counted HoT types amount, not stacks
        Unit::AuraList const& RejorRegr = pVictim->GetAurasByType(SPELL_AURA_PERIODIC_HEAL);
        for(Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
            if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID &&
                (*i)->GetCasterGuid() == GetObjectGuid())
                ++ownHotCount;

        if (ownHotCount)
        {
            DoneTotalMod *= 1.2f;                          // base bonus at HoTs

            if (Aura* glyph = GetAura(62971, EFFECT_INDEX_0))// Glyph of Nourish
                DoneTotalMod *= (glyph->GetModifier()->m_amount * ownHotCount + 100.0f) / 100.0f;
        }
    }

    // Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit  = SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto));

    // apply ap bonus and benefit affected by spell power implicit coeffs and spell level penalties
    DoneTotal = SpellBonusWithCoeffs(spellProto, DoneTotal, DoneAdvertisedBenefit, 0, damagetype, true, 1.88f);

    // use float as more appropriate for negative values and percent applying
    float heal = (healamount + DoneTotal * int32(stack))*DoneTotalMod;
    // apply spellmod to Done amount
    if(Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, heal);

    return heal < 0 ? 0 : uint32(heal);
}

/**
 * Calculates target part of healing spell bonuses,
 * will be called on each tick for periodic damage over time auras
 */
uint32 Unit::SpellHealingBonusTaken(Unit *pCaster, SpellEntry const *spellProto, int32 healamount, DamageEffectType damagetype, uint32 stack)
{
    float  TakenTotalMod = 1.0f;

    // Healing taken percent
    float minval = float(GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
    if (damagetype == DOT)
    {
        // overwrite max SPELL_AURA_MOD_HEALING_PCT if greater negative effect
        float minDotVal = float(GetMaxNegativeAuraModifier(SPELL_AURA_MOD_PERIODIC_HEAL));
        minval = (minDotVal < minval) ? minDotVal : minval;
    }
    if (minval)
        TakenTotalMod *= (100.0f + minval) / 100.0f;

    float maxval = float(GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
    // no SPELL_AURA_MOD_PERIODIC_HEAL positive cases
    if (maxval)
        TakenTotalMod *= (100.0f + maxval) / 100.0f;

    // No heal amount for this class spells
    if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE)
    {
        healamount = int32(healamount * TakenTotalMod);
        return healamount < 0 ? 0 : healamount;
    }

    // Healing Done
    // Done total percent damage auras
    int32  TakenTotal = 0;

    // Taken fixed damage bonus auras
    int32 TakenAdvertisedBenefit = SpellBaseHealingBonusTaken(GetSpellSchoolMask(spellProto));

    // apply benefit affected by spell power implicit coeffs and spell level penalties
    TakenTotal = SpellBonusWithCoeffs(spellProto, TakenTotal, TakenAdvertisedBenefit, 0, damagetype, false, 1.88f);

    AuraList const& mHealingGet= GetAurasByType(SPELL_AURA_MOD_HEALING_RECEIVED);
    for(AuraList::const_iterator i = mHealingGet.begin(); i != mHealingGet.end(); ++i)
        if ((*i)->isAffectedOnSpell(spellProto))
            TakenTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;

    // use float as more appropriate for negative values and percent applying
    float heal = (healamount + TakenTotal * int32(stack)) * TakenTotalMod;

    return heal < 0 ? 0 : uint32(heal);
}

int32 Unit::SpellBaseHealingBonusDone(SpellSchoolMask schoolMask)
{
    int32 AdvertisedBenefit = 0;

    AuraList const& mHealingDone = GetAurasByType(SPELL_AURA_MOD_HEALING_DONE);
    for(AuraList::const_iterator i = mHealingDone.begin();i != mHealingDone.end(); ++i)
        if(!(*i)->GetModifier()->m_miscvalue || ((*i)->GetModifier()->m_miscvalue & schoolMask) != 0)
            AdvertisedBenefit += (*i)->GetModifier()->m_amount;

    // Healing bonus of spirit, intellect and strength
    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        AdvertisedBenefit +=((Player*)this)->GetBaseSpellPowerBonus();

        // Healing bonus from stats
        AuraList const& mHealingDoneOfStatPercent = GetAurasByType(SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT);
        for(AuraList::const_iterator i = mHealingDoneOfStatPercent.begin();i != mHealingDoneOfStatPercent.end(); ++i)
        {
            // stat used dependent from misc value (stat index)
            Stats usedStat = Stats((*i)->GetSpellProto()->EffectMiscValue[(*i)->GetEffIndex()]);
            AdvertisedBenefit += int32(GetStat(usedStat) * (*i)->GetModifier()->m_amount / 100.0f);
        }

        // ... and attack power
        AuraList const& mHealingDonebyAP = GetAurasByType(SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER);
        for(AuraList::const_iterator i = mHealingDonebyAP.begin();i != mHealingDonebyAP.end(); ++i)
            if ((*i)->GetModifier()->m_miscvalue & schoolMask)
                AdvertisedBenefit += int32(GetTotalAttackPowerValue(BASE_ATTACK) * (*i)->GetModifier()->m_amount / 100.0f);
    }
    return AdvertisedBenefit;
}

int32 Unit::SpellBaseHealingBonusTaken(SpellSchoolMask schoolMask)
{
    int32 AdvertisedBenefit = 0;
    AuraList const& mDamageTaken = GetAurasByType(SPELL_AURA_MOD_HEALING);
    for(AuraList::const_iterator i = mDamageTaken.begin();i != mDamageTaken.end(); ++i)
        if ((*i)->GetModifier()->m_miscvalue & schoolMask)
            AdvertisedBenefit += (*i)->GetModifier()->m_amount;

    return AdvertisedBenefit;
}

bool Unit::IsImmunedToDamage(SpellSchoolMask shoolMask)
{
    //If m_immuneToSchool type contain this school type, IMMUNE damage.
    SpellImmuneList const& schoolList = m_spellImmune[IMMUNITY_SCHOOL];
    for (SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
        if (itr->type & shoolMask)
            return true;

    //If m_immuneToDamage type contain magic, IMMUNE damage.
    SpellImmuneList const& damageList = m_spellImmune[IMMUNITY_DAMAGE];
    for (SpellImmuneList::const_iterator itr = damageList.begin(); itr != damageList.end(); ++itr)
        if (itr->type & shoolMask)
            return true;

    return false;
}

bool Unit::IsImmuneToSpell(SpellEntry const* spellInfo)
{
    if (!spellInfo)
        return false;

    //TODO add spellEffect immunity checks!, player with flag in bg is immune to immunity buffs from other friendly players!
    //SpellImmuneList const& dispelList = m_spellImmune[IMMUNITY_EFFECT];

    SpellImmuneList const& dispelList = m_spellImmune[IMMUNITY_DISPEL];
    for(SpellImmuneList::const_iterator itr = dispelList.begin(); itr != dispelList.end(); ++itr)
        if (itr->type == spellInfo->Dispel)
            return true;

    if (!(spellInfo->AttributesEx & SPELL_ATTR_EX_UNAFFECTED_BY_SCHOOL_IMMUNE) &&         // unaffected by school immunity
        !(spellInfo->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY))              // can remove immune (by dispell or immune it)
    {
        SpellImmuneList const& schoolList = m_spellImmune[IMMUNITY_SCHOOL];
        for(SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
            if (!(IsPositiveSpell(itr->spellId) && IsPositiveSpell(spellInfo->Id)) &&
                (itr->type & GetSpellSchoolMask(spellInfo)))
                return true;
    }

    if(uint32 mechanic = spellInfo->Mechanic)
    {
        SpellImmuneList const& mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for(SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
            if (itr->type == mechanic)
                return true;

        AuraList const& immuneAuraApply = GetAurasByType(SPELL_AURA_MECHANIC_IMMUNITY_MASK);
        for(AuraList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
            if ((*iter)->GetModifier()->m_miscvalue & (1 << (mechanic-1)))
                return true;
    }

    return false;
}

bool Unit::IsImmuneToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index) const
{
    //If m_immuneToEffect type contain this effect type, IMMUNE effect.
    uint32 effect = spellInfo->Effect[index];
    SpellImmuneList const& effectList = m_spellImmune[IMMUNITY_EFFECT];
    for (SpellImmuneList::const_iterator itr = effectList.begin(); itr != effectList.end(); ++itr)
        if (itr->type == effect)
            return true;

    if(uint32 mechanic = spellInfo->EffectMechanic[index])
    {
        SpellImmuneList const& mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for (SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
            if (itr->type == mechanic)
                return true;

        AuraList const& immuneAuraApply = GetAurasByType(SPELL_AURA_MECHANIC_IMMUNITY_MASK);
        for(AuraList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
            if ((*iter)->GetModifier()->m_miscvalue & (1 << (mechanic-1)))
                return true;
    }

    if(uint32 aura = spellInfo->EffectApplyAuraName[index])
    {
        SpellImmuneList const& list = m_spellImmune[IMMUNITY_STATE];
        for(SpellImmuneList::const_iterator itr = list.begin(); itr != list.end(); ++itr)
            if (itr->type == aura)
                return true;

        // Check for immune to application of harmful magical effects
        AuraList const& immuneAuraApply = GetAurasByType(SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL);
        for(AuraList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
            if (spellInfo->Dispel == DISPEL_MAGIC &&                                      // Magic debuff
                ((*iter)->GetModifier()->m_miscvalue & GetSpellSchoolMask(spellInfo)) &&  // Check school
                !IsPositiveEffect(spellInfo->Id, index))                                  // Harmful
                return true;

        AuraList const& immuneMechanicAuraApply = GetAurasByType(SPELL_AURA_MECHANIC_IMMUNITY_MASK);
        for(AuraList::const_iterator i = immuneMechanicAuraApply.begin(); i != immuneMechanicAuraApply.end(); ++i)
            if ((spellInfo->EffectMechanic[index] & (*i)->GetMiscValue() ||
                spellInfo->Mechanic & (*i)->GetMiscValue()) ||
                ((*i)->GetId() == 46924 &&                                                // Bladestorm Immunity
                spellInfo->EffectMechanic[index] & IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK ||
                spellInfo->Mechanic & IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK))
                return true;
    }

    return false;
}

bool Unit::IsDamageToThreatSpell(SpellEntry const * spellInfo) const
{
    if (!spellInfo)
        return false;

    uint32 family = spellInfo->SpellFamilyName;
    uint64 flags = spellInfo->SpellFamilyFlags;

    if ((family == 5 && flags == 256) ||                    //Searing Pain
        (family == 6 && flags == 8192) ||                   //Mind Blast
        (family == 11 && flags == 1048576))                 //Earth Shock
        return true;

    return false;
}

/**
 * Calculates caster part of melee damage bonuses,
 * also includes different bonuses dependent from target auras
 */
uint32 Unit::MeleeDamageBonusDone(Unit *pVictim, uint32 pdamage,WeaponAttackType attType, SpellEntry const *spellProto, DamageEffectType damagetype, uint32 stack)
{
    if (!pVictim || pdamage == 0 || (spellProto && spellProto->AttributesEx6 & SPELL_ATTR_EX6_NO_DMG_MODS))
        return pdamage;

    // differentiate for weapon damage based spells
    bool isWeaponDamageBasedSpell = !(spellProto && (damagetype == DOT || IsSpellHaveEffect(spellProto, SPELL_EFFECT_SCHOOL_DAMAGE)));
    Item*  pWeapon          = GetTypeId() == TYPEID_PLAYER ? ((Player*)this)->GetWeaponForAttack(attType,true,false) : NULL;
    uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
    uint32 schoolMask       = spellProto ? spellProto->SchoolMask : GetMeleeDamageSchoolMask();

    // FLAT damage bonus auras
    // =======================
    int32 DoneFlat  = 0;
    int32 APbonus   = 0;

    // ..done flat, already included in weapon damage based spells
    if (!isWeaponDamageBasedSpell)
    {
        AuraList const& mModDamageDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);
        for(AuraList::const_iterator i = mModDamageDone.begin(); i != mModDamageDone.end(); ++i)
        {
            if (((*i)->GetModifier()->m_miscvalue & schoolMask &&                                   // schoolmask has to fit with the intrinsic spell school
                (*i)->GetModifier()->m_miscvalue & GetMeleeDamageSchoolMask() &&                    // AND schoolmask has to fit with weapon damage school (essential for non-physical spells)
                ((*i)->GetSpellProto()->EquippedItemClass == -1) ||                                 // general, weapon independent
                (pWeapon && pWeapon->IsFitToSpellRequirements((*i)->GetSpellProto()))))             // OR used weapon fits aura requirements
            {
                DoneFlat += (*i)->GetModifier()->m_amount;
            }
        }
    }

    // ..done flat (by creature type mask)
    DoneFlat += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE, creatureTypeMask);

    // ..done flat (base at attack power for marked target and base at attack power for creature type)
    if (attType == RANGED_ATTACK)
    {
        APbonus += pVictim->GetTotalAuraModifier(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS);
        APbonus += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS, creatureTypeMask);
    }
    else
    {
        APbonus += pVictim->GetTotalAuraModifier(SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS);
        APbonus += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS, creatureTypeMask);
    }

    // PERCENT damage auras
    // ====================
    float DonePercent   = 1.0f;

    // ..done pct, already included in weapon damage based spells
    if(!isWeaponDamageBasedSpell)
    {
        AuraList const& mModDamagePercentDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        for(AuraList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
        {
            if (((*i)->GetModifier()->m_miscvalue & schoolMask &&                                   // schoolmask has to fit with the intrinsic spell school
                (*i)->GetModifier()->m_miscvalue & GetMeleeDamageSchoolMask() &&                    // AND schoolmask has to fit with weapon damage school (essential for non-physical spells)
                ((*i)->GetSpellProto()->EquippedItemClass == -1) ||                                 // general, weapon independent
                (pWeapon && pWeapon->IsFitToSpellRequirements((*i)->GetSpellProto()))))             // OR used weapon fits aura requirements
            {
                DonePercent *= ((*i)->GetModifier()->m_amount+100.0f) / 100.0f;
            }
        }

        if (attType == OFF_ATTACK)
            DonePercent *= GetModifierValue(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT);                    // no school check required
    }

    // ..done pct (by creature type mask)
    DonePercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS, creatureTypeMask);

    // special dummys/class scripts and other effects
    // =============================================
    Unit *owner = GetOwner();
    if (!owner)
        owner = this;

    // ..done (class scripts)
    if(spellProto)
    {
        AuraList const& mOverrideClassScript= owner->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for(AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
        {
            if (!(*i)->isAffectedOnSpell(spellProto))
                continue;

            switch((*i)->GetModifier()->m_miscvalue)
            {
                // Tundra Stalker
                // Merciless Combat
                case 7277:
                {
                    // Merciless Combat
                    if ((*i)->GetSpellProto()->SpellIconID == 2656)
                    {
                        if(pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                            DonePercent *= (100.0f+(*i)->GetModifier()->m_amount)/100.0f;
                    }
                    else // Tundra Stalker
                    {
                        // Frost Fever (target debuff)
                        if (pVictim->GetAura(SPELL_AURA_MOD_MELEE_HASTE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0000000000000000), 0x00000002))
                            DonePercent *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                        break;
                    }
                    break;
                }
                case 7293: // Rage of Rivendare
                {
                    if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0200000000000000)))
                        DonePercent *= ((*i)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1)*2+100.0f)/100.0f;
                    break;
                }
                // Marked for Death
                case 7598:
                case 7599:
                case 7600:
                case 7601:
                case 7602:
                {
                    if (pVictim->GetAura(SPELL_AURA_MOD_STALKED, SPELLFAMILY_HUNTER, UI64LIT(0x0000000000000400)))
                        DonePercent *= ((*i)->GetModifier()->m_amount+100.0f)/100.0f;
                    break;
                }
            }
        }
    }

    // .. done (class scripts)
    AuraList const& mclassScritAuras = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for(AuraList::const_iterator i = mclassScritAuras.begin(); i != mclassScritAuras.end(); ++i)
    {
        switch((*i)->GetMiscValue())
        {
            // Dirty Deeds
            case 6427:
            case 6428:
                if(pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                {
                    Aura* eff0 = GetAura((*i)->GetId(), EFFECT_INDEX_0);
                    if (!eff0 || (*i)->GetEffIndex() != EFFECT_INDEX_1)
                    {
                        sLog.outError("Spell structure of DD (%u) changed.",(*i)->GetId());
                        continue;
                    }

                    // effect 0 have expected value but in negative state
                    DonePercent *= (-eff0->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
        }
    }

    if (spellProto)
    {
        // Frost Strike
        if (spellProto->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && spellProto->SpellFamilyFlags & UI64LIT(0x0000000400000000))
        {
            // search disease
            bool found = false;
            Unit::SpellAuraHolderMap const& auras = pVictim->GetSpellAuraHolderMap();
            for(Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr!=auras.end(); ++itr)
            {
                if(itr->second->GetSpellProto()->Dispel == DISPEL_DISEASE)
                {
                    found = true;
                    break;
                }
            }

            if(found)
            {
                // search for Glacier Rot dummy aura
                Unit::AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->EffectMiscValue[(*i)->GetEffIndex()] == 7244)
                    {
                        DonePercent *= ((*i)->GetModifier()->m_amount+100.0f) / 100.0f;
                        break;
                    }
                }
            }
        }
        // Glyph of Steady Shot (Steady Shot check)
        else if (spellProto->SpellFamilyName == SPELLFAMILY_HUNTER && spellProto->SpellFamilyFlags & UI64LIT(0x0000000100000000))
        {
            // search for glyph dummy aura
            if (Aura *aur = GetDummyAura(56826))
                // check for Serpent Sting at target
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_HUNTER, UI64LIT(0x0000000000004000)))
                    DonePercent *= (aur->GetModifier()->m_amount+100.0f) / 100.0f;
        }
     }

    // final calculation
    // =================

    float DoneTotal = 0.0f;

    // scaling of non weapon based spells
    if (!isWeaponDamageBasedSpell)
    {
        // apply ap bonus and benefit affected by spell power implicit coeffs and spell level penalties
        DoneTotal = SpellBonusWithCoeffs(spellProto, DoneTotal, DoneFlat, APbonus, damagetype, true);
    }
    // weapon damage based spells
    else if( APbonus || DoneFlat )
    {
        bool normalized = spellProto ? IsSpellHaveEffect(spellProto, SPELL_EFFECT_NORMALIZED_WEAPON_DMG) : false;
        DoneTotal += int32(APbonus / 14.0f * GetAPMultiplier(attType,normalized));

        // for weapon damage based spells we still have to apply damage done percent mods
        // (that are already included into pdamage) to not-yet included DoneFlat
        // e.g. from doneVersusCreature, apBonusVs...
        UnitMods unitMod;
        switch(attType)
        {
            default:
            case BASE_ATTACK:   unitMod = UNIT_MOD_DAMAGE_MAINHAND; break;
            case OFF_ATTACK:    unitMod = UNIT_MOD_DAMAGE_OFFHAND;  break;
            case RANGED_ATTACK: unitMod = UNIT_MOD_DAMAGE_RANGED;   break;
        }

        DoneTotal += DoneFlat;

        DoneTotal *= GetModifierValue(unitMod, TOTAL_PCT);
    }

    float tmpDamage = float(int32(pdamage) + DoneTotal * int32(stack)) * DonePercent;

    // apply spellmod to Done damage
    if(spellProto)
    {
        if(Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, tmpDamage);
    }

    // bonus result can be negative
    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}

/**
 * Calculates target part of melee damage bonuses,
 * will be called on each tick for periodic damage over time auras
 */
uint32 Unit::MeleeDamageBonusTaken(Unit *pCaster, uint32 pdamage,WeaponAttackType attType, SpellEntry const *spellProto, DamageEffectType damagetype, uint32 stack)
{
    if (!pCaster)
        return pdamage;

    if (pdamage == 0)
        return pdamage;

    // differentiate for weapon damage based spells
    bool isWeaponDamageBasedSpell = !(spellProto && (damagetype == DOT || IsSpellHaveEffect(spellProto, SPELL_EFFECT_SCHOOL_DAMAGE)));
    uint32 schoolMask       = spellProto ? spellProto->SchoolMask : GetMeleeDamageSchoolMask();
    uint32 mechanicMask     = spellProto ? GetAllSpellMechanicMask(spellProto) : 0;

    // Shred and Maul also have bonus as MECHANIC_BLEED damages
    if (spellProto && spellProto->SpellFamilyName==SPELLFAMILY_DRUID && spellProto->SpellFamilyFlags & UI64LIT(0x00008800))
        mechanicMask |= (1 << (MECHANIC_BLEED-1));

    // FLAT damage bonus auras
    // =======================
    int32 TakenFlat = 0;

    // ..taken flat (base at attack power for marked target and base at attack power for creature type)
    if (attType == RANGED_ATTACK)
        TakenFlat += GetTotalAuraModifier(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN);
    else
        TakenFlat += GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN);

    // ..taken flat (by school mask)
    TakenFlat += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_DAMAGE_TAKEN, schoolMask);

    // PERCENT damage auras
    // ====================
    float TakenPercent  = 1.0f;

    // ..taken pct (by school mask)
    TakenPercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, schoolMask);

    // ..taken pct (by mechanic mask)
    TakenPercent *= GetTotalAuraMultiplierByMiscValueForMask(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT,mechanicMask);

    // ..taken pct (melee/ranged)
    if(attType == RANGED_ATTACK)
        TakenPercent *= GetTotalAuraMultiplier(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT);
    else
        TakenPercent *= GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT);

    if (spellProto)
    {
        // ..taken pct (from caster spells)
        AuraList const& mOwnerTaken = GetAurasByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
        for(AuraList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
        {
            if ((*i)->GetCasterGUID() == pCaster->GetGUID() && (*i)->isAffectedOnSpell(spellProto))
                TakenPercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
        }
    }

    // ..taken pct (aoe avoidance)
    if(spellProto && IsAreaOfEffectSpell(spellProto))
    {
        TakenPercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE, schoolMask);
        if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
            TakenPercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_PET_AOE_DAMAGE_AVOIDANCE, schoolMask);
    }

    // special dummys/class scripts and other effects
    // =============================================

    // .. taken (dummy auras)
    AuraList const& mDummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    if (!mDummyAuras.empty())
    for(AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
    {
      if ((*i)->GetId())
        switch((*i)->GetSpellProto()->SpellIconID)
        {
            //Cheat Death
            case 2109:
                if((*i)->GetModifier()->m_miscvalue & SPELL_SCHOOL_MASK_NORMAL)
                {
                    if(GetTypeId() != TYPEID_PLAYER)
                        continue;

                    float mod = ((Player*)this)->GetRatingBonusValue(CR_CRIT_TAKEN_MELEE)*(-8.0f);
                    if (mod < float((*i)->GetModifier()->m_amount))
                        mod = float((*i)->GetModifier()->m_amount);

                    TakenPercent *= (mod + 100.0f) / 100.0f;
                }
                break;
        }
    }

    // final calculation
    // =================

    // scaling of non weapon based spells
    if (!isWeaponDamageBasedSpell)
    {
        // apply benefit affected by spell power implicit coeffs and spell level penalties
        TakenFlat = SpellBonusWithCoeffs(spellProto, 0, TakenFlat, 0, damagetype, false);
    }

    float tmpDamage = float(int32(pdamage) + TakenFlat * int32(stack)) * TakenPercent;

    // bonus result can be negative
    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}

void Unit::ApplySpellImmune(uint32 spellId, uint32 op, uint32 type, bool apply)
{
    if (apply)
    {
        for (SpellImmuneList::iterator itr = m_spellImmune[op].begin(), next; itr != m_spellImmune[op].end(); itr = next)
        {
            next = itr; ++next;
            if(itr->type == type)
            {
                m_spellImmune[op].erase(itr);
                next = m_spellImmune[op].begin();
            }
        }
        SpellImmune Immune;
        Immune.spellId = spellId;
        Immune.type = type;
        m_spellImmune[op].push_back(Immune);
    }
    else
    {
        for (SpellImmuneList::iterator itr = m_spellImmune[op].begin(); itr != m_spellImmune[op].end(); ++itr)
        {
            if(itr->spellId == spellId)
            {
                m_spellImmune[op].erase(itr);
                break;
            }
        }
    }
}

void Unit::ApplySpellDispelImmunity(const SpellEntry * spellProto, DispelType type, bool apply)
{
    ApplySpellImmune(spellProto->Id,IMMUNITY_DISPEL, type, apply);

    // such dispell type should not remove auras but only return visibility
    if(type == DISPEL_STEALTH || type == DISPEL_INVISIBILITY)
        return;

    if (apply && spellProto->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        RemoveAurasWithDispelType(type);
}

float Unit::GetWeaponProcChance() const
{
    // normalized proc chance for weapon attack speed
    // (odd formula...)
    if (isAttackReady(BASE_ATTACK))
        return (GetAttackTime(BASE_ATTACK) * 1.8f / 1000.0f);
    else if (haveOffhandWeapon() && isAttackReady(OFF_ATTACK))
        return (GetAttackTime(OFF_ATTACK) * 1.6f / 1000.0f);

    return 0.0f;
}

float Unit::GetPPMProcChance(uint32 WeaponSpeed, float PPM) const
{
    // proc per minute chance calculation
    if (PPM <= 0.0f)
        return 0.0f;
    return WeaponSpeed * PPM / 600.0f;                      // result is chance in percents (probability = Speed_in_sec * (PPM / 60))
}

void Unit::Mount(uint32 mount, uint32 spellId, uint32 vehicleId, uint32 creatureEntry)
{
    if (!mount)
        return;

    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOUNTING);

    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, mount);

    SetFlag( UNIT_FIELD_FLAGS, UNIT_FLAG_MOUNT );

    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Called by Taxi system / GM command
        if (!spellId)
            ((Player*)this)->UnsummonPetTemporaryIfAny();
        // Called by mount aura
        else if (SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId))
        {
            // Flying case (Unsummon any pet)
            if (IsSpellHaveAura(spellInfo, SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED))
                ((Player*)this)->UnsummonPetTemporaryIfAny();
            // Normal case (Unsummon only permanent pet)
            else if (Pet* pet = GetPet())
            {
                if (pet->IsPermanentPetFor((Player*)this) && !((Player*)this)->InArena() &&
                    sWorld.getConfig(CONFIG_BOOL_PET_UNSUMMON_AT_MOUNT))
                {
                    ((Player*)this)->UnsummonPetTemporaryIfAny();
                }
                else
                    pet->ApplyModeFlags(PET_MODE_DISABLE_ACTIONS,true);
            }
            else if (Pet* minipet = GetMiniPet())
            {
                if (sWorld.getConfig(CONFIG_BOOL_PET_UNSUMMON_AT_MOUNT))
                    minipet->Unsummon(PET_SAVE_AS_DELETED, this);
            }
        }

        if (vehicleId)
        {
            if (CreateVehicleKit(vehicleId))
            {
                GetVehicleKit()->Reset();

                // Send others that we now have a vehicle
                WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, 8+4);
                data << GetPackGUID();
                data << uint32(vehicleId);
                SendMessageToSet(&data, true);

                data.Initialize(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, 0);
                ((Player*)this)->GetSession()->SendPacket(&data);

                // mounts can also have accessories
                GetVehicleKit()->InstallAllAccessories(creatureEntry);
            }
        }
    }
}

void Unit::Unmount()
{
    if (!IsMounted())
        return;

    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_MOUNTED);

    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_MOUNT);

    WorldPacket data(SMSG_DISMOUNT, 8);
    data << GetPackGUID();
    SendMessageToSet(&data, true);

    // only resummon old pet if the player is already added to a map
    // this prevents adding a pet to a not created map which would otherwise cause a crash
    // (it could probably happen when logging in after a previous crash)
    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(Pet* pet = GetPet())
            pet->ApplyModeFlags(PET_MODE_DISABLE_ACTIONS,false);
        else
            ((Player*)this)->ResummonPetTemporaryUnSummonedIfAny();
    }

    if (GetTypeId() == TYPEID_PLAYER && GetVehicleKit())
    {
        // Send other players that we are no longer a vehicle
        WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, 8+4);
        data << GetPackGUID();
        data << uint32(0);
        ((Player*)this)->SendMessageToSet(&data, true);

        RemoveVehicleKit();
    }
}

void Unit::SetInCombatWith(Unit* enemy)
{
    Unit* eOwner = enemy->GetCharmerOrOwnerOrSelf();
    if (eOwner->IsPvP())
    {
        SetInCombatState(true,enemy);
        return;
    }

    //check for duel
    if (eOwner->GetTypeId() == TYPEID_PLAYER && ((Player*)eOwner)->duel)
    {
        Unit const* myOwner = GetCharmerOrOwnerOrSelf();
        if(((Player const*)eOwner)->duel->opponent == myOwner)
        {
            SetInCombatState(true,enemy);
            return;
        }
    }

    SetInCombatState(false,enemy);
}

void Unit::SetInCombatState(bool PvP, Unit* enemy)
{
    // only alive units can be in combat
    if (!isAlive())
        return;

    if (PvP)
        m_CombatTimer = 5000;

    bool creatureNotInCombat = GetTypeId()==TYPEID_UNIT && !HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

    if (isCharmed() || (GetTypeId()!=TYPEID_PLAYER && ((Creature*)this)->IsPet()))
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_IN_COMBAT);

    // interrupt all delayed non-combat casts
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
        if (Spell* spell = GetCurrentSpell(CurrentSpellTypes(i)))
            if (IsNonCombatSpell(spell->m_spellInfo))
                InterruptSpell(CurrentSpellTypes(i),false);

    if (creatureNotInCombat)
    {
        // should probably be removed for the attacked (+ it's party/group) only, not global
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);

        // client does not handle this state on it's own (reset to default at LoadCreatureAddon)
        if (getStandState() == UNIT_STAND_STATE_CUSTOM)
            SetStandState(UNIT_STAND_STATE_STAND);

        if (((Creature*)this)->AI())
            ((Creature*)this)->AI()->EnterCombat(enemy);

        if (InstanceData* mapInstance = GetInstanceData())
            mapInstance->OnCreatureEnterCombat((Creature*)this);
    }
}

void Unit::ClearInCombat()
{
    m_CombatTimer = 0;
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

    if(isCharmed() || (GetTypeId()!=TYPEID_PLAYER && ((Creature*)this)->IsPet()))
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_IN_COMBAT);

    // Player's state will be cleared in Player::UpdateContestedPvP
    if (GetTypeId() != TYPEID_PLAYER)
    {
        if (((Creature*)this)->GetCreatureInfo()->unit_flags & UNIT_FLAG_OOC_NOT_ATTACKABLE)
            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);

        clearUnitState(UNIT_STAT_ATTACK_PLAYER);
    }
    else
        ((Player*)this)->UpdatePotionCooldown();
}

bool Unit::isTargetableForAttack(bool inverseAlive /*=false*/) const
{
    if (GetTypeId()==TYPEID_PLAYER && ((Player *)this)->isGameMaster())
        return false;

    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE))
        return false;

    // to be removed if unit by any reason enter combat
    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE))
        return false;

    // inversealive is needed for some spells which need to be casted at dead targets (aoe)
    if (isAlive() == inverseAlive)
        return false;

    return IsInWorld() && !hasUnitState(UNIT_STAT_DIED) && !IsTaxiFlying();
}

int32 Unit::ModifyHealth(int32 dVal)
{
    int32 gain = 0;

    if(dVal==0)
        return 0;

    int32 curHealth = (int32)GetHealth();

    int32 val = dVal + curHealth;
    if(val <= 0)
    {
        SetHealth(0);
        return -curHealth;
    }

    int32 maxHealth = (int32)GetMaxHealth();

    if(val < maxHealth)
    {
        SetHealth(val);
        gain = val - curHealth;
    }
    else if(curHealth != maxHealth)
    {
        SetHealth(maxHealth);
        gain = maxHealth - curHealth;
    }

    return gain;
}

int32 Unit::ModifyPower(Powers power, int32 dVal)
{
    int32 gain = 0;

    if(dVal==0)
        return 0;

    int32 curPower = (int32)GetPower(power);

    int32 val = dVal + curPower;
    if(val <= 0)
    {
        SetPower(power,0);
        return -curPower;
    }

    int32 maxPower = (int32)GetMaxPower(power);

    if(val < maxPower)
    {
        SetPower(power,val);
        gain = val - curPower;
    }
    else if(curPower != maxPower)
    {
        SetPower(power,maxPower);
        gain = maxPower - curPower;
    }

    return gain;
}

bool Unit::isVisibleForOrDetect(Unit const* u, WorldObject const* viewPoint, bool detect, bool inVisibleList, bool is3dDistance) const
{
    if(!u || !IsInMap(u))
        return false;

    // Always can see self
    if (u==this)
        return true;

    // player visible for other player if not logout and at same transport
    // including case when player is out of world
    bool at_same_transport =
        GetTypeId() == TYPEID_PLAYER &&  u->GetTypeId()==TYPEID_PLAYER &&
        !((Player*)this)->GetSession()->PlayerLogout() && !((Player*)u)->GetSession()->PlayerLogout() &&
        !((Player*)this)->GetSession()->PlayerLoading() && !((Player*)u)->GetSession()->PlayerLoading() &&
        ((Player*)this)->GetTransport() && ((Player*)this)->GetTransport() == ((Player*)u)->GetTransport();

    // not in world
    if(!at_same_transport && (!IsInWorld() || !u->IsInWorld()))
        return false;

    // forbidden to seen (at GM respawn command)
    if(m_Visibility==VISIBILITY_RESPAWN)
        return false;

    Map& _map = *u->GetMap();
    // Grid dead/alive checks
    if (u->GetTypeId()==TYPEID_PLAYER)
    {
        // non visible at grid for any stealth state
        if(!IsVisibleInGridForPlayer((Player *)u))
            return false;

        // if player is dead then he can't detect anyone in any cases
        if(!u->isAlive())
            detect = false;
    }
    else
    {
        // all dead creatures/players not visible for any creatures
        if(!u->isAlive() || !isAlive())
            return false;
    }

    // different visible distance checks
    if (u->IsTaxiFlying())                                  // what see player in flight
    {
        // use object grey distance for all (only see objects any way)
        if (!IsWithinDistInMap(viewPoint,World::GetMaxVisibleDistanceInFlight()+(inVisibleList ? World::GetVisibleObjectGreyDistance() : 0.0f), is3dDistance))
            return false;
    }
    else if(!isAlive())                                     // distance for show body
    {
        if (!IsWithinDistInMap(viewPoint,World::GetMaxVisibleDistanceForObject()+(inVisibleList ? World::GetVisibleObjectGreyDistance() : 0.0f), is3dDistance))
            return false;
    }
    else if(!at_same_transport)                             // distance for show player/pet/creature (no transport case)
    {
        // Any units far than max visible distance for viewer or not in our map are not visible too
        if (!IsWithinDistInMap(viewPoint, _map.GetVisibilityDistance() + (inVisibleList ? World::GetVisibleUnitGreyDistance() : 0.0f), is3dDistance))
            return false;
    }

    // always seen by owner
    if (GetCharmerOrOwnerGuid() == u->GetObjectGuid())
        return true;

    // isInvisibleForAlive() those units can only be seen by dead or if other
    // unit is also invisible for alive.. if an isinvisibleforalive unit dies we
    // should be able to see it too
    if (u->isAlive() && isAlive() && isInvisibleForAlive() != u->isInvisibleForAlive())
        if (u->GetTypeId() != TYPEID_PLAYER || !((Player *)u)->isGameMaster())
            return false;

    // Visible units, always are visible for all units, except for units under invisibility and phases
    if (m_Visibility == VISIBILITY_ON && u->m_invisibilityMask==0 && InSamePhase(u))
        return true;

    // GMs see any players, not higher GMs and all units in any phase
    if (u->GetTypeId() == TYPEID_PLAYER && ((Player *)u)->isGameMaster())
    {
        if(GetTypeId() == TYPEID_PLAYER)
            return ((Player *)this)->GetSession()->GetSecurity() <= ((Player *)u)->GetSession()->GetSecurity();
        else
            return true;
    }

    // non faction visibility non-breakable for non-GMs
    if (m_Visibility == VISIBILITY_OFF)
        return false;

    // Arena visibility before arena start
    if (GetTypeId() == TYPEID_PLAYER && HasAura(32727)) // Arena Preparation
        if (Player * p_target = ((Unit*)u)->GetCharmerOrOwnerPlayerOrPlayerItself())
            return ((Player*)this)->GetBGTeam() == p_target->GetBGTeam();

    // phased visibility (both must phased in same way)
    if(!InSamePhase(u))
        return false;

    // raw invisibility
    bool invisible = (m_invisibilityMask != 0 || u->m_invisibilityMask !=0);

    // detectable invisibility case
    if( invisible && (
        // Invisible units, always are visible for units under same invisibility type
        (m_invisibilityMask & u->m_invisibilityMask)!=0 ||
        // Invisible units, always are visible for unit that can detect this invisibility (have appropriate level for detect)
        u->canDetectInvisibilityOf(this) ||
        // Units that can detect invisibility always are visible for units that can be detected
        canDetectInvisibilityOf(u) ))
    {
        invisible = false;
    }

    // special cases for always overwrite invisibility/stealth
    if(invisible || m_Visibility == VISIBILITY_GROUP_STEALTH)
    {
        // non-hostile case
        if (!u->IsHostileTo(this))
        {
            // player see other player with stealth/invisibility only if he in same group or raid or same team (raid/team case dependent from conf setting)
            if(GetTypeId()==TYPEID_PLAYER && u->GetTypeId()==TYPEID_PLAYER)
            {
                if(((Player*)this)->IsGroupVisibleFor(((Player*)u)))
                    return true;

                // else apply same rules as for hostile case (detecting check for stealth)
            }
        }
        // hostile case
        else
        {
            // Hunter mark functionality
            AuraList const& aurasstalked = GetAurasByType(SPELL_AURA_MOD_STALKED);
            for(AuraList::const_iterator iter = aurasstalked.begin(); iter != aurasstalked.end(); ++iter)
                if ((*iter)->GetCasterGuid() == u->GetObjectGuid())
                    return true;

            // Flare functionality
            AuraList const& aurasimunity = GetAurasByType(SPELL_AURA_DISPEL_IMMUNITY);
            for(AuraList::const_iterator iter = aurasimunity.begin(); iter != aurasimunity.end(); ++iter)
                if((*iter)->GetMiscValue() == uint8(invisible ? DISPEL_INVISIBILITY : DISPEL_STEALTH))
                    return true;

            // else apply detecting check for stealth
        }

        // none other cases for detect invisibility, so invisible
        if(invisible)
            return false;

        // else apply stealth detecting check
    }

    // unit got in stealth in this moment and must ignore old detected state
    if (m_Visibility == VISIBILITY_GROUP_NO_DETECT)
        return false;

    // GM invisibility checks early, invisibility if any detectable, so if not stealth then visible
    if (m_Visibility != VISIBILITY_GROUP_STEALTH)
        return true;

    // NOW ONLY STEALTH CASE

    //if in non-detect mode then invisible for unit
    //mobs always detect players (detect == true)... return 'false' for those mobs which have (detect == false)
    //players detect players only in Player::HandleStealthedUnitsDetection()
    if (!detect)
        return (u->GetTypeId() == TYPEID_PLAYER) ? ((Player*)u)->HaveAtClient(this) : false;

    // Special cases

    // If is attacked then stealth is lost, some creature can use stealth too
    if( !getAttackers().empty() )
        return true;

    // If there is collision rogue is seen regardless of level difference
    if (IsWithinDist(u,0.24f))
        return true;

    //If a mob or player is stunned he will not be able to detect stealth
    if (u->hasUnitState(UNIT_STAT_STUNNED) && (u != this))
        return false;

    // set max ditance
    float visibleDistance = (u->GetTypeId() == TYPEID_PLAYER) ? MAX_PLAYER_STEALTH_DETECT_RANGE : ((Creature const*)u)->GetAttackDistance(this);

    //Always invisible from back (when stealth detection is on), also filter max distance cases
    bool isInFront = viewPoint->isInFrontInMap(this, visibleDistance);
    if(!isInFront)
        return false;

    // if doesn't have stealth detection (Shadow Sight), then check how stealthy the unit is, otherwise just check los
    if(!u->HasAuraType(SPELL_AURA_DETECT_STEALTH))
    {
        //Calculation if target is in front

        //Visible distance based on stealth value (stealth rank 4 300MOD, 10.5 - 3 = 7.5)
        visibleDistance = 10.5f - (GetTotalAuraModifier(SPELL_AURA_MOD_STEALTH)/100.0f);

        //Visible distance is modified by
        //-Level Diff (every level diff = 1.0f in visible distance)
        visibleDistance += int32(u->GetLevelForTarget(this)) - int32(GetLevelForTarget(u));

        //This allows to check talent tree and will add addition stealth dependent on used points)
        int32 stealthMod = GetTotalAuraModifier(SPELL_AURA_MOD_STEALTH_LEVEL);
        if(stealthMod < 0)
            stealthMod = 0;

        //-Stealth Mod(positive like Master of Deception) and Stealth Detection(negative like paranoia)
        //based on wowwiki every 5 mod we have 1 more level diff in calculation
        visibleDistance += (int32(u->GetTotalAuraModifier(SPELL_AURA_MOD_STEALTH_DETECT)) - stealthMod)/5.0f;
        visibleDistance = visibleDistance > MAX_PLAYER_STEALTH_DETECT_RANGE ? MAX_PLAYER_STEALTH_DETECT_RANGE : visibleDistance;

        // recheck new distance
        if(visibleDistance <= 0 || !IsWithinDist(viewPoint,visibleDistance))
            return false;
    }

    // Now check is target visible with LoS
    float ox,oy,oz;
    viewPoint->GetPosition(ox,oy,oz);
    return IsWithinLOS(ox,oy,oz);
}

void Unit::SetVisibility(UnitVisibility x)
{
    m_Visibility = x;

    if(IsInWorld())
    {
        // some auras requires visible target
        if(m_Visibility == VISIBILITY_GROUP_NO_DETECT || m_Visibility == VISIBILITY_OFF)
        {
            static const AuraType auratypes[] = {SPELL_AURA_BIND_SIGHT, SPELL_AURA_FAR_SIGHT, SPELL_AURA_NONE};
            for (AuraType const* type = &auratypes[0]; *type != SPELL_AURA_NONE; ++type)
            {
                AuraList& alist = m_modAuras[*type];
                if(alist.empty())
                    continue;

                for (AuraList::iterator it = alist.begin(); it != alist.end();)
                {
                    Aura* aura = (*it);
                    Unit* owner = aura->GetCaster();

                    if (!owner || !isVisibleForOrDetect(owner,this,false))
                    {
                        alist.erase(it);
                        RemoveAura(aura);
                        it = alist.begin();
                    }
                    else
                        ++it;
                }
            }
        }

        Map *m = GetMap();

        if(GetTypeId()==TYPEID_PLAYER)
            m->PlayerRelocation((Player*)this,GetPositionX(),GetPositionY(),GetPositionZ(),GetOrientation());
        else
            m->CreatureRelocation((Creature*)this,GetPositionX(),GetPositionY(),GetPositionZ(),GetOrientation());

        GetViewPoint().Event_ViewPointVisibilityChanged();
    }
}

bool Unit::canDetectInvisibilityOf(Unit const* u) const
{
    if (uint32 mask = (m_detectInvisibilityMask & u->m_invisibilityMask))
    {
        for(int32 i = 0; i < 32; ++i)
        {
            if (((1 << i) & mask)==0)
                continue;

            // find invisibility level
            int32 invLevel = 0;
            Unit::AuraList const& iAuras = u->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
            for(Unit::AuraList::const_iterator itr = iAuras.begin(); itr != iAuras.end(); ++itr)
                if ((*itr)->GetModifier()->m_miscvalue==i && invLevel < (*itr)->GetModifier()->m_amount)
                    invLevel = (*itr)->GetModifier()->m_amount;

            // find invisibility detect level
            int32 detectLevel = 0;
            Unit::AuraList const& dAuras = GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
            for(Unit::AuraList::const_iterator itr = dAuras.begin(); itr != dAuras.end(); ++itr)
                if ((*itr)->GetModifier()->m_miscvalue==i && detectLevel < (*itr)->GetModifier()->m_amount)
                    detectLevel = (*itr)->GetModifier()->m_amount;

            if (i==6 && GetTypeId()==TYPEID_PLAYER)         // special drunk detection case
                detectLevel = ((Player*)this)->GetDrunkValue();

            if (invLevel <= detectLevel)
                return true;
        }
    }

    return false;
}

struct UpdateWalkModeHelper
{
    explicit UpdateWalkModeHelper(Unit* _source) : source(_source) {}
    void operator()(Unit* unit) const { unit->UpdateWalkMode(source, true); }
    Unit* source;
};

void Unit::UpdateWalkMode(Unit* source, bool self)
{
    if (GetTypeId() == TYPEID_PLAYER)
        CallForAllControlledUnits(UpdateWalkModeHelper(source), CONTROLLED_PET|CONTROLLED_GUARDIANS|CONTROLLED_CHARM|CONTROLLED_MINIPET);
    else if (self)
    {
        bool on = source->GetTypeId() == TYPEID_PLAYER
            ? ((Player*)source)->HasMovementFlag(MOVEFLAG_WALK_MODE)
            : ((Creature*)source)->HasSplineFlag(SPLINEFLAG_WALKMODE);

        if (on)
        {
            if (((Creature*)this)->IsPet() && hasUnitState(UNIT_STAT_FOLLOW))
                ((Creature*)this)->AddSplineFlag(SPLINEFLAG_WALKMODE);
        }
        else
        {
            if (((Creature*)this)->IsPet())
                ((Creature*)this)->RemoveSplineFlag(SPLINEFLAG_WALKMODE);
        }
    }
    else
        CallForAllControlledUnits(UpdateWalkModeHelper(source), CONTROLLED_PET|CONTROLLED_GUARDIANS|CONTROLLED_CHARM|CONTROLLED_MINIPET);
}

void Unit::UpdateSpeed(UnitMoveType mtype, bool forced, float ratio)
{
    // not in combat pet have same speed as owner
    switch(mtype)
    {
        case MOVE_RUN:
        case MOVE_WALK:
        case MOVE_SWIM:
            if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet() && hasUnitState(UNIT_STAT_FOLLOW))
            {
                if(Unit* owner = GetOwner())
                {
                    SetSpeedRate(mtype, owner->GetSpeedRate(mtype), forced);
                    return;
                }
            }
            break;
        default:
            break;
    }

    int32 main_speed_mod  = 0;
    float stack_bonus     = 1.0f;
    float non_stack_bonus = 1.0f;

    switch(mtype)
    {
        case MOVE_WALK:
            if (GetTypeId() == TYPEID_UNIT)
            {
                ratio *= ((Creature*)this)->GetCreatureInfo()->speed_walk;
                SetSpeedRate(mtype, ratio, forced);
            }
            return;
        case MOVE_RUN:
        {
            if (GetTypeId() == TYPEID_UNIT)
                ratio *= ((Creature*)this)->GetCreatureInfo()->speed_run;

            if (IsMounted()) // Use on mount auras
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK))/100.0f;
            }
            else
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_SPEED_ALWAYS);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPEED_NOT_STACK))/100.0f;
            }
            break;
        }
        case MOVE_RUN_BACK:
            return;
        case MOVE_SWIM:
        {
            main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SWIM_SPEED);
            break;
        }
        case MOVE_SWIM_BACK:
            return;
        case MOVE_FLIGHT:
        {
            if (IsMounted()) // Use on mount auras
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_STACKING);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_NOT_STACKING))/100.0f;
            }
            else             // Use not mount (shapeshift for example) auras (should stack)
            {
                main_speed_mod  = GetTotalAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_FLIGHT_SPEED_STACKING);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING))/100.0f;
            }
            break;
        }
        case MOVE_FLIGHT_BACK:
            return;
        default:
            sLog.outError("Unit::UpdateSpeed: Unsupported move type (%d)", mtype);
            return;
    }

    // Remove Druid Dash bonus if not in Cat Form
    if (GetShapeshiftForm() != FORM_CAT)
    {
        AuraList const& speed_increase_auras = GetAurasByType(SPELL_AURA_MOD_INCREASE_SPEED);
        for(AuraList::const_iterator itr = speed_increase_auras.begin(); itr != speed_increase_auras.end(); ++itr)
        {
            const SpellEntry* aura_proto = (*itr)->GetSpellProto();
            if (aura_proto->SpellFamilyName == SPELLFAMILY_DRUID && aura_proto->SpellIconID == 959)
            {
                main_speed_mod -= (*itr)->GetModifier()->m_amount;
                break;
            }
        }
    }

    float bonus = non_stack_bonus > stack_bonus ? non_stack_bonus : stack_bonus;
    // now we ready for speed calculation
    float speed  = main_speed_mod ? bonus*(100.0f + main_speed_mod)/100.0f : bonus;

    switch(mtype)
    {
        case MOVE_RUN:
        case MOVE_SWIM:
        case MOVE_FLIGHT:
        {
            // Normalize speed by 191 aura SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED if need
            // TODO: possible affect only on MOVE_RUN
            if(int32 normalization = GetMaxPositiveAuraModifier(SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED))
            {
                // Use speed from aura
                float max_speed = normalization / baseMoveSpeed[mtype];
                if (speed > max_speed)
                    speed = max_speed;
            }
            break;
        }
        default:
            break;
    }

    // for creature case, we check explicit if mob searched for assistance
    if (GetTypeId() == TYPEID_UNIT)
    {
        if (((Creature*)this)->HasSearchedAssistance())
            speed *= 0.66f;                                 // best guessed value, so this will be 33% reduction. Based off initial speed, mob can then "run", "walk fast" or "walk".
    }
    // for player case, we look for some custom rates
    else
    {
        if (getDeathState() == CORPSE)
            speed *= sWorld.getConfig(((Player*)this)->InBattleGround() ? CONFIG_FLOAT_GHOST_RUN_SPEED_BG : CONFIG_FLOAT_GHOST_RUN_SPEED_WORLD);
    }

    // Apply strongest slow aura mod to speed
    int32 slow = GetMaxNegativeAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED);
    if (slow)
    {
        speed *=(100.0f + slow)/100.0f;
        float min_speed = (float)GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MINIMUM_SPEED) / 100.0f;
        if (speed < min_speed)
            speed = min_speed;
    }

    if (GetTypeId() == TYPEID_UNIT)
    {
        switch(mtype)
        {
            case MOVE_RUN:
                speed *= ((Creature*)this)->GetCreatureInfo()->speed_run;
                break;
            case MOVE_WALK:
                speed *= ((Creature*)this)->GetCreatureInfo()->speed_walk;
                break;
            default:
                break;
        }
    }

    SetSpeedRate(mtype, speed * ratio, forced);
}

float Unit::GetSpeed( UnitMoveType mtype ) const
{
    return m_speed_rate[mtype]*baseMoveSpeed[mtype];
}

struct SetSpeedRateHelper
{
    explicit SetSpeedRateHelper(UnitMoveType _mtype, bool _forced) : mtype(_mtype), forced(_forced) {}
    void operator()(Unit* unit) const { unit->UpdateSpeed(mtype,forced); }
    UnitMoveType mtype;
    bool forced;
};

void Unit::SetSpeedRate(UnitMoveType mtype, float rate, bool forced)
{
    if (rate < 0)
        rate = 0.0f;

    // Update speed only on change
    if (m_speed_rate[mtype] == rate)
        return;

    m_speed_rate[mtype] = rate;

    propagateSpeedChange();

    WorldPacket data;
    if(!forced)
    {
        switch(mtype)
        {
            case MOVE_WALK:
                data.Initialize(MSG_MOVE_SET_WALK_SPEED, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_RUN:
                data.Initialize(MSG_MOVE_SET_RUN_SPEED, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_RUN_BACK:
                data.Initialize(MSG_MOVE_SET_RUN_BACK_SPEED, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_SWIM:
                data.Initialize(MSG_MOVE_SET_SWIM_SPEED, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_SWIM_BACK:
                data.Initialize(MSG_MOVE_SET_SWIM_BACK_SPEED, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_TURN_RATE:
                data.Initialize(MSG_MOVE_SET_TURN_RATE, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_FLIGHT:
                data.Initialize(MSG_MOVE_SET_FLIGHT_SPEED, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_FLIGHT_BACK:
                data.Initialize(MSG_MOVE_SET_FLIGHT_BACK_SPEED, 8+4+2+4+4+4+4+4+4+4);
                break;
            case MOVE_PITCH_RATE:
                data.Initialize(MSG_MOVE_SET_PITCH_RATE, 8+4+2+4+4+4+4+4+4+4);
                break;
            default:
                sLog.outError("Unit::SetSpeedRate: Unsupported move type (%d), data not sent to client.",mtype);
                return;
        }

        data << GetPackGUID();
        data << uint32(0);                                  // movement flags
        data << uint16(0);                                  // unk flags
        data << uint32(WorldTimer::getMSTime());
        data << float(GetPositionX());
        data << float(GetPositionY());
        data << float(GetPositionZ());
        data << float(GetOrientation());
        data << uint32(0);                                  // fall time
        data << float(GetSpeed(mtype));
        SendMessageToSet( &data, true );
    }
    else
    {
        if(GetTypeId() == TYPEID_PLAYER)
        {
            // register forced speed changes for WorldSession::HandleForceSpeedChangeAck
            // and do it only for real sent packets and use run for run/mounted as client expected
            ++((Player*)this)->m_forced_speed_changes[mtype];
        }

        switch(mtype)
        {
            case MOVE_WALK:
                data.Initialize(SMSG_FORCE_WALK_SPEED_CHANGE, 16);
                break;
            case MOVE_RUN:
                data.Initialize(SMSG_FORCE_RUN_SPEED_CHANGE, 17);
                break;
            case MOVE_RUN_BACK:
                data.Initialize(SMSG_FORCE_RUN_BACK_SPEED_CHANGE, 16);
                break;
            case MOVE_SWIM:
                data.Initialize(SMSG_FORCE_SWIM_SPEED_CHANGE, 16);
                break;
            case MOVE_SWIM_BACK:
                data.Initialize(SMSG_FORCE_SWIM_BACK_SPEED_CHANGE, 16);
                break;
            case MOVE_TURN_RATE:
                data.Initialize(SMSG_FORCE_TURN_RATE_CHANGE, 16);
                break;
            case MOVE_FLIGHT:
                data.Initialize(SMSG_FORCE_FLIGHT_SPEED_CHANGE, 16);
                break;
            case MOVE_FLIGHT_BACK:
                data.Initialize(SMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE, 16);
                break;
            case MOVE_PITCH_RATE:
                data.Initialize(SMSG_FORCE_PITCH_RATE_CHANGE, 16);
                break;
            default:
                sLog.outError("Unit::SetSpeedRate: Unsupported move type (%d), data not sent to client.",mtype);
                return;
        }
        data << GetPackGUID();
        data << (uint32)0;                                  // moveEvent, NUM_PMOVE_EVTS = 0x39
        if (mtype == MOVE_RUN)
            data << uint8(0);                               // new 2.1.0
        data << float(GetSpeed(mtype));
        SendMessageToSet( &data, true );
    }

    CallForAllControlledUnits(SetSpeedRateHelper(mtype,forced), CONTROLLED_PET|CONTROLLED_GUARDIANS|CONTROLLED_CHARM|CONTROLLED_MINIPET);
}

void Unit::SetHover(bool on)
{
    if(on)
        CastSpell(this, 11010, true);
    else
        RemoveAurasDueToSpell(11010);
}

void Unit::SetDeathState(DeathState s)
{
    if (s != ALIVE && s!= JUST_ALIVED)
    {
        ExitVehicle();
        CombatStop();
        DeleteThreatList();
        ClearComboPointHolders();                           // any combo points pointed to unit lost at it death

        if(IsNonMeleeSpellCasted(false))
            InterruptNonMeleeSpells(false);
    }

    if (s == JUST_DIED)
    {
        RemoveAllAurasOnDeath();
        RemoveGuardians();
        RemoveMiniPet();
        UnsummonAllTotems();

        // after removing a Fearaura (in RemoveAllAurasOnDeath)
        // Unit::SetFeared is called and makes that creatures attack player again
        if (GetTypeId() == TYPEID_UNIT)
        {
            clearUnitState(UNIT_STAT_MOVING);

            GetMap()->CreatureRelocation((Creature*)this, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
            SendMonsterMove(GetPositionX(), GetPositionY(), GetPositionZ(), SPLINETYPE_NORMAL, SPLINEFLAG_WALKMODE, 0);
        }
        else
        {
            if (!IsStopped())
                StopMoving();
        }

        ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, false);
        ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, false);
        // remove aurastates allowing special moves
        ClearAllReactives();
        ClearDiminishings();
        ProcDamageAndSpell(this, PROC_FLAG_NONE, PROC_FLAG_ON_DEATH, PROC_EX_NONE, 0);

        if (GetVehicleKit())
            GetVehicleKit()->RemoveAllPassengers();
    }
    else if(s == JUST_ALIVED)
    {
        RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE); // clear skinnable for creature and player (at battleground)
    }

    if (m_deathState != ALIVE && s == ALIVE)
    {
        //_ApplyAllAuraMods();
    }
    m_deathState = s;
}

/*########################################
########                          ########
########       AGGRO SYSTEM       ########
########                          ########
########################################*/

bool Unit::CanHaveThreatList() const
{
    // only creatures can have threat list
    if (GetTypeId() != TYPEID_UNIT)
        return false;

    // only alive units can have threat list
    if (!isAlive())
        return false;

    Creature const* creature = ((Creature const*)this);

    // totems can not have threat list
    if (creature->IsTotem())
        return false;

    // pets can not have a threat list, unless they are controlled by a creature
    if (creature->IsPet() && creature->GetOwnerGuid().IsPlayer())
        return false;

    // charmed units can not have a threat list if charmed by player
    if (creature->GetCharmerGuid().IsPlayer())
        return false;

    // Is it correct?
    if (isCharmed())
        return false;

    return true;
}

//======================================================================

float Unit::ApplyTotalThreatModifier(float threat, SpellSchoolMask schoolMask)
{
    if (!HasAuraType(SPELL_AURA_MOD_THREAT))
        return threat;

    if (schoolMask == SPELL_SCHOOL_MASK_NONE)
        return threat;

    SpellSchools school = GetFirstSchoolInMask(schoolMask);

    return threat * m_threatModifier[school];
}

//======================================================================

void Unit::AddThreat(Unit* pVictim, float threat /*= 0.0f*/, bool crit /*= false*/, SpellSchoolMask schoolMask /*= SPELL_SCHOOL_MASK_NONE*/, SpellEntry const *threatSpell /*= NULL*/)
{
    // Only mobs can manage threat lists
    if (CanHaveThreatList())
        m_ThreatManager.addThreat(pVictim, threat, crit, schoolMask, threatSpell);
}

//======================================================================

void Unit::DeleteThreatList()
{
    if (CanHaveThreatList() && !m_ThreatManager.isThreatListEmpty())
        SendThreatClear();

    m_ThreatManager.clearReferences();
}

//======================================================================

void Unit::TauntApply(Unit* taunter)
{
    MANGOS_ASSERT(GetTypeId() == TYPEID_UNIT);

    if (!taunter || (taunter->GetTypeId() == TYPEID_PLAYER && ((Player*)taunter)->isGameMaster()))
        return;

    if (!CanHaveThreatList())
        return;

    Unit *target = getVictim();

    if (target && target == taunter)
        return;

    SetInFront(taunter);

    if (((Creature*)this)->AI())
        ((Creature*)this)->AI()->AttackStart(taunter);

    m_ThreatManager.tauntApply(taunter);
}

//======================================================================

void Unit::TauntFadeOut(Unit *taunter)
{
    MANGOS_ASSERT(GetTypeId() == TYPEID_UNIT);

    if (!taunter || (taunter->GetTypeId() == TYPEID_PLAYER && ((Player*)taunter)->isGameMaster()))
        return;

    if (!CanHaveThreatList())
        return;

    Unit *target = getVictim();

    if (!target || target != taunter)
        return;

    if (m_ThreatManager.isThreatListEmpty())
    {
        if (((Creature*)this)->AI())
            ((Creature*)this)->AI()->EnterEvadeMode();

        if (InstanceData* mapInstance = GetInstanceData())
            mapInstance->OnCreatureEvade((Creature*)this);

        return;
    }

    m_ThreatManager.tauntFadeOut(taunter);
    target = m_ThreatManager.getHostileTarget();

    if (target && target != taunter)
    {
        SetInFront(target);

        if (((Creature*)this)->AI())
            ((Creature*)this)->AI()->AttackStart(target);
    }
}

//======================================================================

bool Unit::SelectHostileTarget()
{
    //function provides main threat functionality
    //next-victim-selection algorithm and evade mode are called
    //threat list sorting etc.

    MANGOS_ASSERT(GetTypeId() == TYPEID_UNIT);

    if (!this->isAlive())
        return false;

    //This function only useful once AI has been initialized
    if (!((Creature*)this)->AI())
        return false;

    Unit* target = NULL;

    // First checking if we have some taunt on us
    const AuraList& tauntAuras = GetAurasByType(SPELL_AURA_MOD_TAUNT);
    if (!tauntAuras.empty())
    {
        Unit* caster;

        // The last taunt aura caster is alive an we are happy to attack him
        if ((caster = tauntAuras.back()->GetCaster()) && caster->isAlive())
            return true;
        else if (tauntAuras.size() > 1)
        {
            // We do not have last taunt aura caster but we have more taunt auras,
            // so find first available target

            // Auras are pushed_back, last caster will be on the end
            AuraList::const_iterator aura = --tauntAuras.end();
            do
            {
                --aura;
                if ((caster = (*aura)->GetCaster()) && caster->IsInMap(this) &&
                    caster->isTargetableForAttack() && caster->isInAccessablePlaceFor((Creature*)this))
                {
                    target = caster;
                    break;
                }
            }while (aura != tauntAuras.begin());
        }
    }

    // No taunt aura or taunt aura caster is dead, standard target selection
    if (!target && !m_ThreatManager.isThreatListEmpty())
        target = m_ThreatManager.getHostileTarget();

    if (target)
    {
        if (!hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_DIED))
        {
            SetInFront(target);
            ((Creature*)this)->AI()->AttackStart(target);
        }
        return true;
    }

    // no target but something prevent go to evade mode
    if (!isInCombat() || HasAuraType(SPELL_AURA_MOD_TAUNT))
        return false;

    // last case when creature don't must go to evade mode:
    // it in combat but attacker not make any damage and not enter to aggro radius to have record in threat list
    // for example at owner command to pet attack some far away creature
    // Note: creature not have targeted movement generator but have attacker in this case
    if (GetMotionMaster()->GetCurrentMovementGeneratorType() != CHASE_MOTION_TYPE)
    {
        for(AttackerSet::const_iterator itr = m_attackers.begin(); itr != m_attackers.end(); ++itr)
        {
            if ((*itr)->IsInMap(this) && (*itr)->isTargetableForAttack() && (*itr)->isInAccessablePlaceFor((Creature*)this))
                return false;
        }
    }

    // enter in evade mode in other case
    ((Creature*)this)->AI()->EnterEvadeMode();

    if (InstanceData* mapInstance = GetInstanceData())
        mapInstance->OnCreatureEvade((Creature*)this);

    return false;
}

//======================================================================
//======================================================================
//======================================================================

int32 Unit::CalculateSpellDamage(Unit const* target, SpellEntry const* spellProto, SpellEffectIndex effect_index, int32 const* effBasePoints)
{
    Player* unitPlayer = (GetTypeId() == TYPEID_PLAYER) ? (Player*)this : NULL;

    uint8 comboPoints = GetComboPoints();

    int32 level = int32(getLevel());
    if (level > (int32)spellProto->maxLevel && spellProto->maxLevel > 0)
        level = (int32)spellProto->maxLevel;
    else if (level < (int32)spellProto->baseLevel)
        level = (int32)spellProto->baseLevel;
    level-= (int32)spellProto->spellLevel;

    float basePointsPerLevel = spellProto->EffectRealPointsPerLevel[effect_index];
    int32 basePoints = effBasePoints ? *effBasePoints - 1 : spellProto->EffectBasePoints[effect_index];
    basePoints += int32(level * basePointsPerLevel);
    int32 randomPoints = int32(spellProto->EffectDieSides[effect_index]);
    float comboDamage = spellProto->EffectPointsPerComboPoint[effect_index];

    switch(randomPoints)
    {
        case 0:                                             // not used
        case 1: basePoints += 1; break;                     // range 1..1
        default:
            // range can have positive (1..rand) and negative (rand..1) values, so order its for irand
            int32 randvalue = (randomPoints >= 1)
                ? irand(1, randomPoints)
                : irand(randomPoints, 1);

            basePoints += randvalue;
            break;
    }

    int32 value = basePoints;

    // random damage
    if (comboDamage != 0 && unitPlayer && target && (target->GetObjectGuid() == unitPlayer->GetComboTargetGuid()))
        value += (int32)(comboDamage * comboPoints);

    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_ALL_EFFECTS, value);

        switch(effect_index)
        {
            case EFFECT_INDEX_0:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT1, value);
                break;
            case EFFECT_INDEX_1:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT2, value);
                break;
            case EFFECT_INDEX_2:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT3, value);
                break;
        }
    }

    if(spellProto->Attributes & SPELL_ATTR_LEVEL_DAMAGE_CALCULATION && spellProto->spellLevel &&
            spellProto->Effect[effect_index] != SPELL_EFFECT_WEAPON_PERCENT_DAMAGE &&
            spellProto->Effect[effect_index] != SPELL_EFFECT_KNOCK_BACK &&
            (spellProto->Effect[effect_index] != SPELL_EFFECT_APPLY_AURA || spellProto->EffectApplyAuraName[effect_index] != SPELL_AURA_MOD_DECREASE_SPEED))
        value = int32(value*0.25f*exp(getLevel()*(70-spellProto->spellLevel)/1000.0f));

    return value;
}

int32 Unit::CalculateBaseSpellDuration(SpellEntry const* spellProto, uint32* periodicTime)
{
    int32 duration = GetSpellDuration(spellProto);

    if (duration < 0)
        return duration;

    if (GetTypeId() == TYPEID_PLAYER)
    {
        int32 maxduration = GetSpellMaxDuration(spellProto);

        if (duration != maxduration)
            duration += int32((maxduration - duration) * ((Player*)this)->GetComboPoints() / 5);
    }

    Player* modOwner = GetSpellModOwner();

    if (modOwner)
    {
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_DURATION, duration);

        if (duration <= 0)
            return 0;
    }

    bool applyHaste = (spellProto->AttributesEx5 & SPELL_ATTR_EX5_AFFECTED_BY_HASTE) != 0;

    if (!applyHaste)
    {
        Unit::AuraList const& mModByHaste = GetAurasByType(SPELL_AURA_MOD_PERIODIC_HASTE);
        for (Unit::AuraList::const_iterator itr = mModByHaste.begin(); itr != mModByHaste.end(); ++itr)
        {
            if ((*itr)->isAffectedOnSpell(spellProto))
            {
                applyHaste = true;
                break;
            }
        }
    }

    uint32 oldDuration = duration;

    // Apply haste to duration
    if (applyHaste)
        duration = int32(duration * GetFloatValue(UNIT_MOD_CAST_SPEED));

    uint32 _periodicTime = periodicTime ? *periodicTime : 0;

    if (_periodicTime)
    {
        if (modOwner)
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_ACTIVATION_TIME, _periodicTime);

        // Calculate new periodic timer
        if (applyHaste)
        {
            int32 ticks = oldDuration / _periodicTime;
            _periodicTime = duration / ticks;
        }

        *periodicTime = _periodicTime;
    }

    return duration;
}

uint32 Unit::CalculateSpellDuration(Unit const* caster, uint32 baseDuration, SpellEntry const* spellProto, SpellEffectIndex effect_index)
{
    int32 mechanic = GetEffectMechanic(spellProto, effect_index);
    // Find total mod value (negative bonus)
    int32 durationMod_always = GetTotalAuraModifierByMiscValue(SPELL_AURA_MECHANIC_DURATION_MOD, mechanic);
    // Modify from SPELL_AURA_MOD_DURATION_OF_EFFECTS_BY_DISPEL aura for negative effects (stack always ?)
    if (!IsPositiveEffect(spellProto->Id, effect_index))
        durationMod_always += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DURATION_OF_EFFECTS_BY_DISPEL, spellProto->Dispel);
    // Find max mod (negative bonus)
    int32 durationMod_not_stack = GetMaxNegativeAuraModifierByMiscValue(SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK, mechanic);

    if (!IsPositiveSpell(spellProto->Id))
        durationMod_always += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DURATION_OF_MAGIC_EFFECTS, spellProto->Dispel);

    int32 durationMod = 0;

    // Select strongest negative mod
    if (durationMod_always > durationMod_not_stack)
        durationMod = durationMod_not_stack;
    else
        durationMod = durationMod_always;

    if (caster == this)
    {
        switch(spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_DRUID:
                if (spellProto->SpellFamilyFlags & UI64LIT(0x100))
                {
                    // Glyph of Thorns
                    if (Aura *aur = GetAura(57862, EFFECT_INDEX_0))
                        baseDuration += aur->GetModifier()->m_amount * MINUTE * IN_MILLISECONDS;
                }
                break;
            case SPELLFAMILY_PALADIN:
                if (spellProto->SpellIconID == 298 && spellProto->SpellFamilyFlags & UI64LIT(0x00000002))
                {
                    // Glyph of Blessing of Might
                    if (Aura *aur = GetAura(57958, EFFECT_INDEX_0))
                        baseDuration += aur->GetModifier()->m_amount * MINUTE * IN_MILLISECONDS;
                }
                else if (spellProto->SpellIconID == 306 && spellProto->SpellFamilyFlags & UI64LIT(0x00010000))
                {
                    // Glyph of Blessing of Wisdom
                    if (Aura *aur = GetAura(57979, EFFECT_INDEX_0))
                        baseDuration += aur->GetModifier()->m_amount * MINUTE * IN_MILLISECONDS;
                }
                break;
            default:
                break;
        }
    }

    if (durationMod != 0)
    {
        int32 duration = int32(int64(baseDuration) * (100+durationMod) / 100);
        return duration < 0 ? 0 : duration;
    }

    return baseDuration;
}

DiminishingLevels Unit::GetDiminishing(DiminishingGroup group)
{
    for(Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if(i->DRGroup != group)
            continue;

        if(!i->hitCount)
            return DIMINISHING_LEVEL_1;

        if (!i->hitTime)
            return DIMINISHING_LEVEL_1;

        // If last spell was casted more than 15 seconds ago - reset the count.
        if (i->stack==0 && WorldTimer::getMSTimeDiff(i->hitTime,WorldTimer::getMSTime()) > 15*IN_MILLISECONDS)
        {
            i->hitCount = DIMINISHING_LEVEL_1;
            return DIMINISHING_LEVEL_1;
        }
        // or else increase the count.
        else
        {
            return DiminishingLevels(i->hitCount);
        }
    }
    return DIMINISHING_LEVEL_1;
}

void Unit::IncrDiminishing(DiminishingGroup group)
{
    // Checking for existing in the table
    for(Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if(i->DRGroup != group)
            continue;
        if(i->hitCount < DIMINISHING_LEVEL_IMMUNE)
            i->hitCount += 1;
        return;
    }
    m_Diminishing.push_back(DiminishingReturn(group,WorldTimer::getMSTime(),DIMINISHING_LEVEL_2));
}

void Unit::ApplyDiminishingToDuration(DiminishingGroup group, int32 &duration,Unit* caster,DiminishingLevels Level, int32 limitduration)
{
    if(duration == -1 || group == DIMINISHING_NONE)
        return;

    // Duration of crowd control abilities on pvp target is limited by 10 sec. (2.2.0)
    if(limitduration > 0 && duration > limitduration)
    {
        // test pet/charm masters instead pets/charmeds
        Unit const* targetOwner = GetCharmerOrOwner();
        Unit const* casterOwner = caster->GetCharmerOrOwner();

        Unit const* target = targetOwner ? targetOwner : this;
        Unit const* source = casterOwner ? casterOwner : caster;

        if(target->GetTypeId() == TYPEID_PLAYER && source->GetTypeId() == TYPEID_PLAYER)
            duration = limitduration;
    }

    float mod = 1.0f;

    // Some diminishings applies to mobs too (for example, Stun)
    if((GetDiminishingReturnsGroupType(group) == DRTYPE_PLAYER && GetTypeId() == TYPEID_PLAYER) || GetDiminishingReturnsGroupType(group) == DRTYPE_ALL)
    {
        DiminishingLevels diminish = Level;
        switch(diminish)
        {
            case DIMINISHING_LEVEL_1: break;
            case DIMINISHING_LEVEL_2: mod = 0.5f; break;
            case DIMINISHING_LEVEL_3: mod = 0.25f; break;
            case DIMINISHING_LEVEL_IMMUNE: mod = 0.0f;break;
            default: break;
        }
    }

    duration = int32(duration * mod);
}

void Unit::ApplyDiminishingAura( DiminishingGroup group, bool apply )
{
    // Checking for existing in the table
    for(Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if(i->DRGroup != group)
            continue;

        if(apply)
            i->stack += 1;
        else if(i->stack)
        {
            i->stack -= 1;
            // Remember time after last aura from group removed
            if (i->stack == 0)
                i->hitTime = WorldTimer::getMSTime();
        }
        break;
    }
}

bool Unit::isVisibleForInState( Player const* u, WorldObject const* viewPoint, bool inVisibleList ) const
{
    return isVisibleForOrDetect(u, viewPoint, false, inVisibleList, false);
}

/// returns true if creature can't be seen by alive units
bool Unit::isInvisibleForAlive() const
{
    if (m_AuraFlags & UNIT_AURAFLAG_ALIVE_INVISIBLE)
        return true;
    // TODO: maybe spiritservices also have just an aura
    return isSpiritService();
}

uint32 Unit::GetCreatureType() const
{
    if(GetTypeId() == TYPEID_PLAYER)
    {
        SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(GetShapeshiftForm());
        if(ssEntry && ssEntry->creatureType > 0)
            return ssEntry->creatureType;
        else
            return CREATURE_TYPE_HUMANOID;
    }
    else
        return ((Creature*)this)->GetCreatureInfo()->type;
}

/*#######################################
########                         ########
########       STAT SYSTEM       ########
########                         ########
#######################################*/

bool Unit::HandleStatModifier(UnitMods unitMod, UnitModifierType modifierType, float amount, bool apply)
{
    if(unitMod >= UNIT_MOD_END || modifierType >= MODIFIER_TYPE_END)
    {
        sLog.outError("ERROR in HandleStatModifier(): nonexistent UnitMods or wrong UnitModifierType!");
        return false;
    }

    float val = 1.0f;

    switch(modifierType)
    {
        case BASE_VALUE:
        case TOTAL_VALUE:
            m_auraModifiersGroup[unitMod][modifierType] += apply ? amount : -amount;
            break;
        case BASE_PCT:
        case TOTAL_PCT:
            if(amount <= -100.0f)                           //small hack-fix for -100% modifiers
                amount = -200.0f;

            val = (100.0f + amount) / 100.0f;
            m_auraModifiersGroup[unitMod][modifierType] *= apply ? val : (1.0f/val);
            break;

        default:
            break;
    }

    if(!CanModifyStats())
        return false;

    switch(unitMod)
    {
        case UNIT_MOD_STAT_STRENGTH:
        case UNIT_MOD_STAT_AGILITY:
        case UNIT_MOD_STAT_STAMINA:
        case UNIT_MOD_STAT_INTELLECT:
        case UNIT_MOD_STAT_SPIRIT:         UpdateStats(GetStatByAuraGroup(unitMod));  break;

        case UNIT_MOD_ARMOR:               UpdateArmor();           break;
        case UNIT_MOD_HEALTH:              UpdateMaxHealth();       break;

        case UNIT_MOD_MANA:
        case UNIT_MOD_RAGE:
        case UNIT_MOD_FOCUS:
        case UNIT_MOD_ENERGY:
        case UNIT_MOD_HAPPINESS:
        case UNIT_MOD_RUNE:
        case UNIT_MOD_RUNIC_POWER:          UpdateMaxPower(GetPowerTypeByAuraGroup(unitMod));          break;

        case UNIT_MOD_RESISTANCE_HOLY:
        case UNIT_MOD_RESISTANCE_FIRE:
        case UNIT_MOD_RESISTANCE_NATURE:
        case UNIT_MOD_RESISTANCE_FROST:
        case UNIT_MOD_RESISTANCE_SHADOW:
        case UNIT_MOD_RESISTANCE_ARCANE:   UpdateResistances(GetSpellSchoolByAuraGroup(unitMod));      break;

        case UNIT_MOD_ATTACK_POWER:        UpdateAttackPowerAndDamage();         break;
        case UNIT_MOD_ATTACK_POWER_RANGED: UpdateAttackPowerAndDamage(true);     break;

        case UNIT_MOD_DAMAGE_MAINHAND:     UpdateDamagePhysical(BASE_ATTACK);    break;
        case UNIT_MOD_DAMAGE_OFFHAND:      UpdateDamagePhysical(OFF_ATTACK);     break;
        case UNIT_MOD_DAMAGE_RANGED:       UpdateDamagePhysical(RANGED_ATTACK);  break;

        default:
            break;
    }

    return true;
}

float Unit::GetModifierValue(UnitMods unitMod, UnitModifierType modifierType) const
{
    if( unitMod >= UNIT_MOD_END || modifierType >= MODIFIER_TYPE_END)
    {
        sLog.outError("attempt to access nonexistent modifier value from UnitMods!");
        return 0.0f;
    }

    if(modifierType == TOTAL_PCT && m_auraModifiersGroup[unitMod][modifierType] <= 0.0f)
        return 0.0f;

    return m_auraModifiersGroup[unitMod][modifierType];
}

float Unit::GetTotalStatValue(Stats stat) const
{
    UnitMods unitMod = UnitMods(UNIT_MOD_STAT_START + stat);

    if(m_auraModifiersGroup[unitMod][TOTAL_PCT] <= 0.0f)
        return 0.0f;

    // value = ((base_value * base_pct) + total_value) * total_pct
    float value  = m_auraModifiersGroup[unitMod][BASE_VALUE] + GetCreateStat(stat);
    value *= m_auraModifiersGroup[unitMod][BASE_PCT];
    value += m_auraModifiersGroup[unitMod][TOTAL_VALUE];
    value *= m_auraModifiersGroup[unitMod][TOTAL_PCT];

    return value;
}

float Unit::GetTotalAuraModValue(UnitMods unitMod) const
{
    if(unitMod >= UNIT_MOD_END)
    {
        sLog.outError("attempt to access nonexistent UnitMods in GetTotalAuraModValue()!");
        return 0.0f;
    }

    if(m_auraModifiersGroup[unitMod][TOTAL_PCT] <= 0.0f)
        return 0.0f;

    float value  = m_auraModifiersGroup[unitMod][BASE_VALUE];
    value *= m_auraModifiersGroup[unitMod][BASE_PCT];
    value += m_auraModifiersGroup[unitMod][TOTAL_VALUE];
    value *= m_auraModifiersGroup[unitMod][TOTAL_PCT];

    return value;
}

SpellSchools Unit::GetSpellSchoolByAuraGroup(UnitMods unitMod) const
{
    SpellSchools school = SPELL_SCHOOL_NORMAL;

    switch(unitMod)
    {
        case UNIT_MOD_RESISTANCE_HOLY:     school = SPELL_SCHOOL_HOLY;          break;
        case UNIT_MOD_RESISTANCE_FIRE:     school = SPELL_SCHOOL_FIRE;          break;
        case UNIT_MOD_RESISTANCE_NATURE:   school = SPELL_SCHOOL_NATURE;        break;
        case UNIT_MOD_RESISTANCE_FROST:    school = SPELL_SCHOOL_FROST;         break;
        case UNIT_MOD_RESISTANCE_SHADOW:   school = SPELL_SCHOOL_SHADOW;        break;
        case UNIT_MOD_RESISTANCE_ARCANE:   school = SPELL_SCHOOL_ARCANE;        break;

        default:
            break;
    }

    return school;
}

Stats Unit::GetStatByAuraGroup(UnitMods unitMod) const
{
    Stats stat = STAT_STRENGTH;

    switch(unitMod)
    {
        case UNIT_MOD_STAT_STRENGTH:    stat = STAT_STRENGTH;      break;
        case UNIT_MOD_STAT_AGILITY:     stat = STAT_AGILITY;       break;
        case UNIT_MOD_STAT_STAMINA:     stat = STAT_STAMINA;       break;
        case UNIT_MOD_STAT_INTELLECT:   stat = STAT_INTELLECT;     break;
        case UNIT_MOD_STAT_SPIRIT:      stat = STAT_SPIRIT;        break;

        default:
            break;
    }

    return stat;
}

Powers Unit::GetPowerTypeByAuraGroup(UnitMods unitMod) const
{
    switch(unitMod)
    {
        case UNIT_MOD_MANA:       return POWER_MANA;
        case UNIT_MOD_RAGE:       return POWER_RAGE;
        case UNIT_MOD_FOCUS:      return POWER_FOCUS;
        case UNIT_MOD_ENERGY:     return POWER_ENERGY;
        case UNIT_MOD_HAPPINESS:  return POWER_HAPPINESS;
        case UNIT_MOD_RUNE:       return POWER_RUNE;
        case UNIT_MOD_RUNIC_POWER:return POWER_RUNIC_POWER;
        default:                  return POWER_MANA;
    }

    return POWER_MANA;
}

float Unit::GetTotalAttackPowerValue(WeaponAttackType attType) const
{
    if (attType == RANGED_ATTACK)
    {
        int32 ap = GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER) + GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MODS);
        if (ap < 0)
            return 0.0f;
        return ap * (1.0f + GetFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER));
    }
    else
    {
        int32 ap = GetInt32Value(UNIT_FIELD_ATTACK_POWER) + GetInt32Value(UNIT_FIELD_ATTACK_POWER_MODS);
        if (ap < 0)
            return 0.0f;
        return ap * (1.0f + GetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER));
    }
}

float Unit::GetWeaponDamageRange(WeaponAttackType attType ,WeaponDamageRange type) const
{
    if (attType == OFF_ATTACK && !haveOffhandWeapon())
        return 0.0f;

    return m_weaponDamage[attType][type];
}

void Unit::SetLevel(uint32 lvl)
{
    SetUInt32Value(UNIT_FIELD_LEVEL, lvl);

    // group update
    if ((GetTypeId() == TYPEID_PLAYER) && ((Player*)this)->GetGroup())
        ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_LEVEL);
}

void Unit::SetHealth(uint32 val)
{
    uint32 maxHealth = GetMaxHealth();
    if(maxHealth < val)
        val = maxHealth;

    SetUInt32Value(UNIT_FIELD_HEALTH, val);

    // group update
    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(((Player*)this)->GetGroup())
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_HP);
    }
    else if(((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_HP);
        }
    }
}

void Unit::SetMaxHealth(uint32 val)
{
    uint32 health = GetHealth();
    SetUInt32Value(UNIT_FIELD_MAXHEALTH, val);

    // group update
    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(((Player*)this)->GetGroup())
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_HP);
    }
    else if(((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_HP);
        }
    }

    if(val < health)
        SetHealth(val);
}

void Unit::SetHealthPercent(float percent)
{
    uint32 newHealth = GetMaxHealth() * percent/100.0f;
    SetHealth(newHealth);
}

void Unit::SetPower(Powers power, uint32 val)
{
    if(GetPower(power) == val)
        return;

    uint32 maxPower = GetMaxPower(power);
    if(maxPower < val)
        val = maxPower;

    SetStatInt32Value(UNIT_FIELD_POWER1 + power, val);

    WorldPacket data(SMSG_POWER_UPDATE);
    data << GetPackGUID();
    data << uint8(power);
    data << uint32(val);
    SendMessageToSet(&data, true);

    // group update
    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(((Player*)this)->GetGroup())
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_POWER);
    }
    else if(((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_POWER);
        }

        // Update the pet's character sheet with happiness damage bonus
        if(pet->getPetType() == HUNTER_PET && power == POWER_HAPPINESS)
        {
            pet->UpdateDamagePhysical(BASE_ATTACK);
        }
    }
}

void Unit::SetMaxPower(Powers power, uint32 val)
{
    uint32 cur_power = GetPower(power);
    SetStatInt32Value(UNIT_FIELD_MAXPOWER1 + power, val);

    // group update
    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(((Player*)this)->GetGroup())
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_POWER);
    }
    else if(((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_POWER);
        }
    }

    if(val < cur_power)
        SetPower(power, val);
}

void Unit::ApplyPowerMod(Powers power, uint32 val, bool apply)
{
    ApplyModUInt32Value(UNIT_FIELD_POWER1+power, val, apply);

    // group update
    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(((Player*)this)->GetGroup())
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_POWER);
    }
    else if(((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_POWER);
        }
    }
}

void Unit::ApplyMaxPowerMod(Powers power, uint32 val, bool apply)
{
    ApplyModUInt32Value(UNIT_FIELD_MAXPOWER1+power, val, apply);

    // group update
    if(GetTypeId() == TYPEID_PLAYER)
    {
        if(((Player*)this)->GetGroup())
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_POWER);
    }
    else if(((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_POWER);
        }
    }
}

void Unit::ApplyAuraProcTriggerDamage( Aura* aura, bool apply )
{
    AuraList& tAuraProcTriggerDamage = m_modAuras[SPELL_AURA_PROC_TRIGGER_DAMAGE];
    if(apply)
        tAuraProcTriggerDamage.push_back(aura);
    else
        tAuraProcTriggerDamage.remove(aura);
}

uint32 Unit::GetCreatePowers( Powers power ) const
{
    switch(power)
    {
        case POWER_HEALTH:      return 0;                   // is it really should be here?
        case POWER_MANA:        return GetCreateMana();
        case POWER_RAGE:        return 1000;
        case POWER_FOCUS:       return (GetTypeId() == TYPEID_PLAYER || !((Creature const*)this)->IsPet() || ((Pet const*)this)->getPetType() != HUNTER_PET ? 0 : 100);
        case POWER_ENERGY:      return 100;
        case POWER_HAPPINESS:   return (GetTypeId() == TYPEID_PLAYER || !((Creature const*)this)->IsPet() || ((Pet const*)this)->getPetType() != HUNTER_PET ? 0 : 1050000);
        case POWER_RUNE:        return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_DEATH_KNIGHT ? 8 : 0);
        case POWER_RUNIC_POWER: return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_DEATH_KNIGHT ? 1000 : 0);
    }

    return 0;
}

void Unit::AddToWorld()
{
    Object::AddToWorld();
}

void Unit::RemoveFromWorld()
{
    // cleanup
    if (IsInWorld())
    {
        Uncharm();
        RemoveNotOwnSingleTargetAuras();
        RemoveGuardians();
        RemoveMiniPet();
        UnsummonAllTotems();
        RemoveAllGameObjects();
        RemoveAllDynObjects();
        CleanupDeletedAuras();
        GetViewPoint().Event_RemovedFromWorld();
    }

    Object::RemoveFromWorld();
}

void Unit::CleanupsBeforeDelete()
{
    if(m_uint32Values)                                      // only for fully created object
    {
        RemoveVehicleKit();
        ExitVehicle();

        InterruptNonMeleeSpells(true);
        m_Events.KillAllEvents(false);                      // non-delatable (currently casted spells) will not deleted now but it will deleted at call in Map::RemoveAllObjectsInRemoveList
        CombatStop();
        ClearComboPointHolders();
        DeleteThreatList();
        if (GetTypeId()==TYPEID_PLAYER)
            getHostileRefManager().setOnlineOfflineState(false);
        else
            getHostileRefManager().deleteReferences();
        RemoveAllAuras(AURA_REMOVE_BY_DELETE);
        GetMotionMaster()->Clear(false);                    // remove different non-standard movement generators.
    }
    WorldObject::CleanupsBeforeDelete();
}

CharmInfo* Unit::InitCharmInfo(Unit *charm)
{
    if(!m_charmInfo)
        m_charmInfo = new CharmInfo(charm);
    return m_charmInfo;
}

CharmInfo::CharmInfo(Unit* unit)
: m_unit(unit), m_CommandState(COMMAND_FOLLOW), m_reactState(REACT_PASSIVE), m_petnumber(0)
{
    for(int i = 0; i < CREATURE_MAX_SPELLS; ++i)
        m_charmspells[i].SetActionAndType(0,ACT_DISABLED);
    m_petnumber = 0;
}

void CharmInfo::InitPetActionBar()
{
    // the first 3 SpellOrActions are attack, follow and stay
    for(uint32 i = 0; i < ACTION_BAR_INDEX_PET_SPELL_START - ACTION_BAR_INDEX_START; ++i)
        SetActionBar(ACTION_BAR_INDEX_START + i,COMMAND_ATTACK - i,ACT_COMMAND);

    // middle 4 SpellOrActions are spells/special attacks/abilities
    for(uint32 i = 0; i < ACTION_BAR_INDEX_PET_SPELL_END-ACTION_BAR_INDEX_PET_SPELL_START; ++i)
        SetActionBar(ACTION_BAR_INDEX_PET_SPELL_START + i,0,ACT_DISABLED);

    // last 3 SpellOrActions are reactions
    for(uint32 i = 0; i < ACTION_BAR_INDEX_END - ACTION_BAR_INDEX_PET_SPELL_END; ++i)
        SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + i,COMMAND_ATTACK - i,ACT_REACTION);
}

void CharmInfo::InitEmptyActionBar()
{
    SetActionBar(ACTION_BAR_INDEX_START,COMMAND_ATTACK,ACT_COMMAND);
    for(uint32 x = ACTION_BAR_INDEX_START+1; x < ACTION_BAR_INDEX_END; ++x)
        SetActionBar(x,0,ACT_PASSIVE);
}

void CharmInfo::InitPossessCreateSpells()
{
    InitEmptyActionBar();                                   //charm action bar

    if(m_unit->GetTypeId() == TYPEID_PLAYER)                //possessed players don't have spells, keep the action bar empty
        return;

    for(uint32 x = 0; x < CREATURE_MAX_SPELLS; ++x)
    {
        if (IsPassiveSpell(((Creature*)m_unit)->m_spells[x]))
            m_unit->CastSpell(m_unit, ((Creature*)m_unit)->m_spells[x], true);
        else
            AddSpellToActionBar(((Creature*)m_unit)->m_spells[x], ACT_PASSIVE);
    }
}

void CharmInfo::InitVehicleCreateSpells()
{
    for (uint32 x = ACTION_BAR_INDEX_START; x < ACTION_BAR_INDEX_END; ++x)
        SetActionBar(x, 0, ActiveStates(0x8 + x));

    for (uint32 x = 0; x < CREATURE_MAX_SPELLS; ++x)
    {
        uint32 spellId = ((Creature*)m_unit)->m_spells[x];

        if (!spellId)
            continue;

        if (IsPassiveSpell(spellId))
            m_unit->CastSpell(m_unit, spellId, true);
        else
            PetActionBar[x].SetAction(spellId);
    }
}

void CharmInfo::InitCharmCreateSpells()
{
    if(m_unit->GetTypeId() == TYPEID_PLAYER)                //charmed players don't have spells
    {
        InitEmptyActionBar();
        return;
    }

    InitPetActionBar();

    for(uint32 x = 0; x < CREATURE_MAX_SPELLS; ++x)
    {
        uint32 spellId = ((Creature*)m_unit)->m_spells[x];

        if(!spellId)
        {
            m_charmspells[x].SetActionAndType(spellId,ACT_DISABLED);
            continue;
        }

        if (IsPassiveSpell(spellId))
        {
            m_unit->CastSpell(m_unit, spellId, true);
            m_charmspells[x].SetActionAndType(spellId,ACT_PASSIVE);
        }
        else
        {
            m_charmspells[x].SetActionAndType(spellId,ACT_DISABLED);

            ActiveStates newstate;
            bool onlyselfcast = true;
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

            if(!spellInfo) onlyselfcast = false;
            for(uint32 i = 0; i < 3 && onlyselfcast; ++i)   //nonexistent spell will not make any problems as onlyselfcast would be false -> break right away
            {
                if(spellInfo->EffectImplicitTargetA[i] != TARGET_SELF && spellInfo->EffectImplicitTargetA[i] != 0)
                    onlyselfcast = false;
            }

            if(onlyselfcast || !IsPositiveSpell(spellId))   // only self cast and spells versus enemies are autocastable
                newstate = ACT_DISABLED;
            else
                newstate = ACT_PASSIVE;

            AddSpellToActionBar(spellId, newstate);
        }
    }
}

bool CharmInfo::AddSpellToActionBar(uint32 spell_id, ActiveStates newstate)
{
    uint32 first_id = sSpellMgr.GetFirstSpellInChain(spell_id);

    // new spell rank can be already listed
    for(uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (uint32 action = PetActionBar[i].GetAction())
        {
            if (PetActionBar[i].IsActionBarForSpell() && sSpellMgr.GetFirstSpellInChain(action) == first_id)
            {
                PetActionBar[i].SetAction(spell_id);
                return true;
            }
        }
    }

    // or use empty slot in other case
    for(uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (!PetActionBar[i].GetAction() && PetActionBar[i].IsActionBarForSpell())
        {
            SetActionBar(i,spell_id,newstate == ACT_DECIDE ? ACT_DISABLED : newstate);
            return true;
        }
    }
    return false;
}

bool CharmInfo::RemoveSpellFromActionBar(uint32 spell_id)
{
    uint32 first_id = sSpellMgr.GetFirstSpellInChain(spell_id);

    for(uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (uint32 action = PetActionBar[i].GetAction())
        {
            if (PetActionBar[i].IsActionBarForSpell() && sSpellMgr.GetFirstSpellInChain(action) == first_id)
            {
                SetActionBar(i,0,ACT_DISABLED);
                return true;
            }
        }
    }

    return false;
}

void CharmInfo::ToggleCreatureAutocast(uint32 spellid, bool apply)
{
    if(IsPassiveSpell(spellid))
        return;

    for(uint32 x = 0; x < CREATURE_MAX_SPELLS; ++x)
        if(spellid == m_charmspells[x].GetAction())
            m_charmspells[x].SetType(apply ? ACT_ENABLED : ACT_DISABLED);
}

void CharmInfo::SetPetNumber(uint32 petnumber, bool statwindow)
{
    m_petnumber = petnumber;
    if(statwindow)
        m_unit->SetUInt32Value(UNIT_FIELD_PETNUMBER, m_petnumber);
    else
        m_unit->SetUInt32Value(UNIT_FIELD_PETNUMBER, 0);
}

void CharmInfo::LoadPetActionBar(const std::string& data )
{
    InitPetActionBar();

    Tokens tokens = StrSplit(data, " ");

    if (tokens.size() != (ACTION_BAR_INDEX_END-ACTION_BAR_INDEX_START)*2)
        return;                                             // non critical, will reset to default

    int index;
    Tokens::iterator iter;
    for(iter = tokens.begin(), index = ACTION_BAR_INDEX_START; index < ACTION_BAR_INDEX_END; ++iter, ++index )
    {
        // use unsigned cast to avoid sign negative format use at long-> ActiveStates (int) conversion
        uint8 type  = (uint8)atol((*iter).c_str());
        ++iter;
        uint32 action = atol((*iter).c_str());

        PetActionBar[index].SetActionAndType(action,ActiveStates(type));

        // check correctness
        if(PetActionBar[index].IsActionBarForSpell() && !sSpellStore.LookupEntry(PetActionBar[index].GetAction()))
            SetActionBar(index,0,ACT_DISABLED);
    }
}

void CharmInfo::BuildActionBar( WorldPacket* data )
{
    for(uint32 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
        *data << uint32(PetActionBar[i].packedData);
}

void CharmInfo::SetSpellAutocast( uint32 spell_id, bool state )
{

    for(int i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if(spell_id == PetActionBar[i].GetAction() && PetActionBar[i].IsActionBarForSpell())
        {
            PetActionBar[i].SetType(state ? ACT_ENABLED : ACT_DISABLED);
            break;
        }
    }
}

void Unit::DoPetAction( Player* owner, uint8 flag, uint32 spellid, ObjectGuid petGuid, ObjectGuid targetGuid)
{
    if ((((Creature*)this)->IsPet() && !((Pet*)this)->IsInWorld()) || !GetCharmInfo())
        return;

    switch(flag)
    {
        case ACT_COMMAND:                                   //0x07
       // Maybe exists some flag that disable it at client side
            if (petGuid.IsVehicle())
                return;

            switch(spellid)
            {
                case COMMAND_STAY:                          //flat=1792  //STAY
                    StopMoving();
                    GetMotionMaster()->Clear(false);
                    GetMotionMaster()->MoveIdle();
                    GetCharmInfo()->SetCommandState( COMMAND_STAY );
                    break;
                case COMMAND_FOLLOW:                        //spellid=1792  //FOLLOW
                    AttackStop();
                    GetMotionMaster()->MoveFollow(owner,PET_FOLLOW_DIST,((Pet*)this)->GetPetFollowAngle());
                    GetCharmInfo()->SetCommandState( COMMAND_FOLLOW );
                    break;
                case COMMAND_ATTACK:                        //spellid=1792  //ATTACK
                {
                    Unit *TargetUnit = owner->GetMap()->GetUnit(targetGuid);
                    if(!TargetUnit)
                        return;

                    // not let attack friendly units.
                    if(owner->IsFriendlyTo(TargetUnit))
                        return;
                    // Not let attack through obstructions
                    if(!IsWithinLOSInMap(TargetUnit))
                        return;

                    // This is true if pet has no target or has target but targets differs.
                    if(getVictim() != TargetUnit)
                    {
                        if (getVictim())
                            AttackStop();

                        if (hasUnitState(UNIT_STAT_CONTROLLED))
                        {
                            Attack(TargetUnit, true);
                            SendPetAIReaction();
                        }
                        else
                        {
                            GetMotionMaster()->Clear();

                            if (((Creature*)this)->AI())
                                ((Creature*)this)->AI()->AttackStart(TargetUnit);

                            // 10% chance to play special pet attack talk, else growl
                            if(((Creature*)this)->IsPet() && ((Pet*)this)->getPetType() == SUMMON_PET && this != TargetUnit && roll_chance_i(10))
                                SendPetTalk((uint32)PET_TALK_ATTACK);
                            else
                            {
                                // 90% chance for pet and 100% chance for charmed creature
                                SendPetAIReaction();
                            }
                        }

                    }
                    break;
                }
                case COMMAND_ABANDON:                       // abandon (hunter pet) or dismiss (summoned pet)
                    if(((Creature*)this)->IsPet())
                    {
                        Pet* p = (Pet*)this;
                        if(p->getPetType() == HUNTER_PET)
                            p->Unsummon(PET_SAVE_AS_DELETED, owner);
                        else
                            //dismissing a summoned pet is like killing them (this prevents returning a soulshard...)
                            p->SetDeathState(CORPSE);
                    }
                    else                                    // charmed
                        owner->Uncharm();
                    break;
                default:
                    sLog.outError("WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
            }
            break;
        case ACT_REACTION:                                  // 0x6
            switch(spellid)
            {
                case REACT_PASSIVE:                         //passive
                case REACT_DEFENSIVE:                       //recovery
                case REACT_AGGRESSIVE:                      //activete
                    GetCharmInfo()->SetReactState( ReactStates(spellid) );
                    break;
            }
            break;
        case ACT_DISABLED:                                  // 0x81    spell (disabled), ignore
        case ACT_PASSIVE:                                   // 0x01
        case ACT_ENABLED:                                   // 0xC1    spell
        {
            Unit* unit_target = NULL;

            if (!targetGuid.IsEmpty())
                unit_target = owner->GetMap()->GetUnit(targetGuid);

            // do not cast unknown spells
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellid );
            if(!spellInfo)
            {
                sLog.outError("WORLD: unknown PET spell id %i", spellid);
                return;
            }

            if (GetCharmInfo() && GetCharmInfo()->GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo))
                return;

            for(int i = 0; i < MAX_EFFECT_INDEX;++i)
            {
                if(spellInfo->EffectImplicitTargetA[i] == TARGET_ALL_ENEMY_IN_AREA || spellInfo->EffectImplicitTargetA[i] == TARGET_ALL_ENEMY_IN_AREA_INSTANT || spellInfo->EffectImplicitTargetA[i] == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
                    return;
            }

            // do not cast not learned spells
            if(!HasSpell(spellid) || IsPassiveSpell(spellid))
                return;

            clearUnitState(UNIT_STAT_MOVING);

            Spell *spell = new Spell(this, spellInfo, false);

            SpellCastResult result = spell->CheckPetCast(unit_target);

            //auto turn to target unless possessed
            if(result == SPELL_FAILED_UNIT_NOT_INFRONT && !HasAuraType(SPELL_AURA_MOD_POSSESS))
            {
                if(unit_target)
                {
                    SetInFront(unit_target);
                    if (unit_target->GetTypeId() == TYPEID_PLAYER)
                        SendCreateUpdateToPlayer( (Player*)unit_target );
                }
                else if(Unit *unit_target2 = spell->m_targets.getUnitTarget())
                {
                    SetInFront(unit_target2);
                    if (unit_target2->GetTypeId() == TYPEID_PLAYER)
                        SendCreateUpdateToPlayer( (Player*)unit_target2 );
                }
                if (Unit* powner = GetCharmerOrOwner())
                    if(powner->GetTypeId() == TYPEID_PLAYER)
                        SendCreateUpdateToPlayer((Player*)powner);
                result = SPELL_CAST_OK;
            }

            if(result == SPELL_CAST_OK)
            {
                ((Creature*)this)->AddCreatureSpellCooldown(spellid);

                unit_target = spell->m_targets.getUnitTarget();

                //10% chance to play special pet attack talk, else growl
                //actually this only seems to happen on special spells, fire shield for imp, torment for voidwalker, but it's stupid to check every spell
                if(((Creature*)this)->IsPet() && (((Pet*)this)->getPetType() == SUMMON_PET) && (this != unit_target) && (urand(0, 100) < 10))
                    SendPetTalk((uint32)PET_TALK_SPECIAL_SPELL);
                else
                    SendPetAIReaction();

                if( unit_target && !owner->IsFriendlyTo(unit_target) && !HasAuraType(SPELL_AURA_MOD_POSSESS))
                {
                    // This is true if pet has no target or has target but targets differs.
                    if (getVictim() != unit_target)
                    {
                        if (getVictim())
                            AttackStop();
                        GetMotionMaster()->Clear();
                        if (((Creature*)this)->AI())
                            ((Creature*)this)->AI()->AttackStart(unit_target);
                    }
                }

                spell->prepare(&(spell->m_targets));
            }
            else
            {
                if(HasAuraType(SPELL_AURA_MOD_POSSESS))
                    Spell::SendCastResult(owner,spellInfo,0,result);
                else
                    SendPetCastFail(spellid, result);

                if (!((Creature*)this)->HasSpellCooldown(spellid))
                    owner->SendClearCooldown(spellid, this);

                spell->finish(false);
                delete spell;
            }
            break;
        }
        default:
            sLog.outError("WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
    }

}

void Unit::DoPetCastSpell( Player *owner, uint8 cast_count, SpellCastTargets* targets, SpellEntry const* spellInfo )
{
    Creature* pet = dynamic_cast<Creature*>(this);

    clearUnitState(UNIT_STAT_MOVING);

    Spell *spell = new Spell(pet, spellInfo, false);
    spell->m_cast_count = cast_count;                       // probably pending spell cast
    spell->m_targets = *targets;

    SpellCastResult result = spell->CheckPetCast(NULL);
    if (result == SPELL_CAST_OK)
    {
        pet->AddCreatureSpellCooldown(spellInfo->Id);
        if (pet->IsPet())
        {
            //10% chance to play special pet attack talk, else growl
            //actually this only seems to happen on special spells, fire shield for imp, torment for voidwalker, but it's stupid to check every spell
            if(((Pet*)pet)->getPetType() == SUMMON_PET && (urand(0, 100) < 10))
                pet->SendPetTalk((uint32)PET_TALK_SPECIAL_SPELL);
            else
                pet->SendPetAIReaction();
        }

        spell->prepare(&(spell->m_targets));
    }
    else
    {
        pet->SendPetCastFail(spellInfo->Id, result);
        if (!pet->HasSpellCooldown(spellInfo->Id))
            owner->SendClearCooldown(spellInfo->Id, pet);

        spell->finish(false);
        delete spell;
    }
}

bool Unit::isFrozen() const
{
    return HasAuraState(AURA_STATE_FROZEN);
}

struct ProcTriggeredData
{
    ProcTriggeredData(SpellProcEventEntry const * _spellProcEvent, SpellAuraHolder* _triggeredByHolder)
        : spellProcEvent(_spellProcEvent), triggeredByHolder(_triggeredByHolder)
        {}
    SpellProcEventEntry const *spellProcEvent;
    SpellAuraHolder* triggeredByHolder;
};

typedef std::list< ProcTriggeredData > ProcTriggeredList;
typedef std::list< uint32> RemoveSpellList;

uint32 createProcExtendMask(SpellNonMeleeDamage *damageInfo, SpellMissInfo missCondition)
{
    uint32 procEx = PROC_EX_NONE;
    // Check victim state
    if (missCondition!=SPELL_MISS_NONE)
    switch (missCondition)
    {
        case SPELL_MISS_MISS:    procEx|=PROC_EX_MISS;   break;
        case SPELL_MISS_RESIST:  procEx|=PROC_EX_RESIST; break;
        case SPELL_MISS_DODGE:   procEx|=PROC_EX_DODGE;  break;
        case SPELL_MISS_PARRY:   procEx|=PROC_EX_PARRY;  break;
        case SPELL_MISS_BLOCK:   procEx|=PROC_EX_BLOCK;  break;
        case SPELL_MISS_EVADE:   procEx|=PROC_EX_EVADE;  break;
        case SPELL_MISS_IMMUNE:  procEx|=PROC_EX_IMMUNE; break;
        case SPELL_MISS_IMMUNE2: procEx|=PROC_EX_IMMUNE; break;
        case SPELL_MISS_DEFLECT: procEx|=PROC_EX_DEFLECT;break;
        case SPELL_MISS_ABSORB:  procEx|=PROC_EX_ABSORB; break;
        case SPELL_MISS_REFLECT: procEx|=PROC_EX_REFLECT;break;
        default:
            break;
    }
    else
    {
        // On block
        if (damageInfo->blocked)
            procEx|=PROC_EX_BLOCK;
        // On absorb
        if (damageInfo->absorb)
            procEx|=PROC_EX_ABSORB;
        // On crit
        if (damageInfo->HitInfo & SPELL_HIT_TYPE_CRIT)
            procEx|=PROC_EX_CRITICAL_HIT;
        else
            procEx|=PROC_EX_NORMAL_HIT;
    }
    return procEx;
}

void Unit::ProcDamageAndSpellFor( bool isVictim, Unit * pTarget, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, SpellEntry const * procSpell, uint32 damage )
{
    // For melee/ranged based attack need update skills and set some Aura states
    if (procFlag & MELEE_BASED_TRIGGER_MASK)
    {
        // Update skills here for players
        if (GetTypeId() == TYPEID_PLAYER)
        {
            // On melee based hit/miss/resist need update skill (for victim and attacker)
            if (procExtra&(PROC_EX_NORMAL_HIT|PROC_EX_MISS|PROC_EX_RESIST))
            {
                if (pTarget->GetTypeId() != TYPEID_PLAYER && pTarget->GetCreatureType() != CREATURE_TYPE_CRITTER)
                    ((Player*)this)->UpdateCombatSkills(pTarget, attType, isVictim);
            }
            // Update defence if player is victim and parry/dodge/block
            if (isVictim && procExtra&(PROC_EX_DODGE|PROC_EX_PARRY|PROC_EX_BLOCK))
                ((Player*)this)->UpdateDefense();
        }
        // If exist crit/parry/dodge/block need update aura state (for victim and attacker)
        if (procExtra & (PROC_EX_CRITICAL_HIT|PROC_EX_PARRY|PROC_EX_DODGE|PROC_EX_BLOCK))
        {
            // for victim
            if (isVictim)
            {
                // if victim and dodge attack
                if (procExtra&PROC_EX_DODGE)
                {
                    //Update AURA_STATE on dodge
                    if (getClass() != CLASS_ROGUE) // skip Rogue Riposte
                    {
                        ModifyAuraState(AURA_STATE_DEFENSE, true);
                        StartReactiveTimer( REACTIVE_DEFENSE );
                    }
                }
                // if victim and parry attack
                if (procExtra & PROC_EX_PARRY)
                {
                    // For Hunters only Counterattack (skip Mongoose bite)
                    if (getClass() == CLASS_HUNTER)
                    {
                        ModifyAuraState(AURA_STATE_HUNTER_PARRY, true);
                        StartReactiveTimer( REACTIVE_HUNTER_PARRY );
                    }
                    else
                    {
                        ModifyAuraState(AURA_STATE_DEFENSE, true);
                        StartReactiveTimer( REACTIVE_DEFENSE );
                    }
                }
                // if and victim block attack
                if (procExtra & PROC_EX_BLOCK)
                {
                    ModifyAuraState(AURA_STATE_DEFENSE,true);
                    StartReactiveTimer( REACTIVE_DEFENSE );
                }
            }
            else //For attacker
            {
                // Overpower on victim dodge
                if (procExtra&PROC_EX_DODGE && GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_WARRIOR)
                {
                    AddComboPoints(pTarget, 1);
                    StartReactiveTimer( REACTIVE_OVERPOWER );
                }
            }
        }
    }

    RemoveSpellList removedSpells;
    ProcTriggeredList procTriggered;
    // Fill procTriggered list
    for(SpellAuraHolderMap::const_iterator itr = GetSpellAuraHolderMap().begin(); itr!= GetSpellAuraHolderMap().end(); ++itr)
    {
        // skip deleted auras (possible at recursive triggered call
        if(itr->second->IsDeleted())
            continue;

        SpellProcEventEntry const* spellProcEvent = NULL;
        if(!IsTriggeredAtSpellProcEvent(pTarget, itr->second, procSpell, procFlag, procExtra, attType, isVictim, spellProcEvent))
           continue;

        itr->second->SetInUse(true);                        // prevent holder deletion
        procTriggered.push_back( ProcTriggeredData(spellProcEvent, itr->second) );
    }

    // Nothing found
    if (procTriggered.empty())
        return;

    // Handle effects proceed this time
    for(ProcTriggeredList::const_iterator i = procTriggered.begin(); i != procTriggered.end(); ++i)
    {
        // Some auras can be deleted in function called in this loop (except first, ofc)
        SpellAuraHolder *triggeredByHolder = i->triggeredByHolder;
        if(triggeredByHolder->IsDeleted())
            continue;

        SpellProcEventEntry const *spellProcEvent = i->spellProcEvent;
        bool useCharges = triggeredByHolder->GetAuraCharges() > 0;
        bool procSuccess = true;
        bool anyAuraProc = false;

        // For players set spell cooldown if need
        uint32 cooldown = 0;
        if (GetTypeId() == TYPEID_PLAYER && spellProcEvent && spellProcEvent->cooldown)
            cooldown = spellProcEvent->cooldown;

        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            Aura *triggeredByAura = triggeredByHolder->GetAuraByEffectIndex(SpellEffectIndex(i));
            if (!triggeredByAura)
                continue;

            Modifier *auraModifier = triggeredByAura->GetModifier();

            if (procSpell)
            {
                if (spellProcEvent)
                {
                    if (spellProcEvent->spellFamilyMask[i] || spellProcEvent->spellFamilyMask2[i])
                    {
                        if ((spellProcEvent->spellFamilyMask[i]  & procSpell->SpellFamilyFlags ) == 0 &&
                            (spellProcEvent->spellFamilyMask2[i] & procSpell->SpellFamilyFlags2) == 0)
                            continue;
                    }
                    // don't check dbc FamilyFlags if schoolMask exists
                    else if (!triggeredByAura->CanProcFrom(procSpell, spellProcEvent->procEx, procExtra, damage != 0, !spellProcEvent->schoolMask))
                        continue;
                }
                else if (!triggeredByAura->CanProcFrom(procSpell, PROC_EX_NONE, procExtra, damage != 0, true))
                    continue;
            }

            SpellAuraProcResult procResult = (*this.*AuraProcHandler[auraModifier->m_auraname])(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown);
            switch (procResult)
            {
                case SPELL_AURA_PROC_CANT_TRIGGER:
                    continue;
                case SPELL_AURA_PROC_FAILED:
                    procSuccess = false;
                    break;
                case SPELL_AURA_PROC_OK:
                    break;
            }

            anyAuraProc = true;
        }

        // Remove charge (aura can be removed by triggers)
        if(useCharges && procSuccess && anyAuraProc && !triggeredByHolder->IsDeleted())
        {
            // If last charge dropped add spell to remove list
            if(triggeredByHolder->DropAuraCharge())
                removedSpells.push_back(triggeredByHolder->GetId());
        }
        // If reflecting with Imp. Spell Reflection - we must also remove auras from the remaining aura's targets
        if (triggeredByHolder->GetId() == 59725)
            if (Unit* pImpSRCaster = triggeredByHolder->GetCaster() )
                if (Group* group = ((Player*)pImpSRCaster)->GetGroup())
                    for(GroupReference *itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                        if (Player* member = itr->getSource())
                            if (Aura* pAura = member->GetAura(59725, EFFECT_INDEX_0) )
                                if (pAura->GetCaster() == pImpSRCaster)
                                    member->RemoveAura(pAura);

        triggeredByHolder->SetInUse(false);
    }

    if (!removedSpells.empty())
    {
        // Sort spells and remove duplicates
        removedSpells.sort();
        removedSpells.unique();
        // Remove auras from removedAuras
        for(RemoveSpellList::const_iterator i = removedSpells.begin(); i != removedSpells.end();++i)
            RemoveAurasDueToSpell(*i);
    }
}

SpellSchoolMask Unit::GetMeleeDamageSchoolMask() const
{
    return SPELL_SCHOOL_MASK_NORMAL;
}

Player* Unit::GetSpellModOwner()
{
    if(GetTypeId()==TYPEID_PLAYER)
        return (Player*)this;
    if(((Creature*)this)->IsPet() || ((Creature*)this)->IsTotem())
    {
        Unit* owner = GetOwner();
        if(owner && owner->GetTypeId()==TYPEID_PLAYER)
            return (Player*)owner;
    }
    return NULL;
}

///----------Pet responses methods-----------------
void Unit::SendPetCastFail(uint32 spellid, SpellCastResult msg)
{
    if(msg == SPELL_CAST_OK)
        return;

    Unit *owner = GetCharmerOrOwner();
    if(!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_PET_CAST_FAILED, 1 + 4 + 1);
    data << uint8(0);                                       // cast count?
    data << uint32(spellid);
    data << uint8(msg);
    // uint32 for some reason
    // uint32 for some reason
    ((Player*)owner)->GetSession()->SendPacket(&data);
}

void Unit::SendPetActionFeedback (uint8 msg)
{
    Unit* owner = GetOwner();
    if(!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_PET_ACTION_FEEDBACK, 1);
    data << uint8(msg);
    ((Player*)owner)->GetSession()->SendPacket(&data);
}

void Unit::SendPetTalk (uint32 pettalk)
{
    Unit* owner = GetOwner();
    if(!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_PET_ACTION_SOUND, 8 + 4);
    data << GetObjectGuid();
    data << uint32(pettalk);
    ((Player*)owner)->GetSession()->SendPacket(&data);
}

void Unit::SendPetAIReaction()
{
    Unit* owner = GetOwner();
    if(!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_AI_REACTION, 8 + 4);
    data << GetObjectGuid();
    data << uint32(AI_REACTION_HOSTILE);
    ((Player*)owner)->GetSession()->SendPacket(&data);
}

///----------End of Pet responses methods----------

void Unit::StopMoving()
{
    clearUnitState(UNIT_STAT_MOVING);

    // not need send any packets if not in world
    if (!IsInWorld())
        return;

    // send explicit stop packet
    // player expected for correct work SPLINEFLAG_WALKMODE
    SendMonsterMove(GetPositionX(), GetPositionY(), GetPositionZ(), SPLINETYPE_STOP, GetTypeId() == TYPEID_PLAYER ? SPLINEFLAG_WALKMODE : SPLINEFLAG_NONE, 0);

    // update position and orientation for near players
    SendHeartBeat(false);
}

void Unit::SetFeared(bool apply, ObjectGuid casterGuid, uint32 spellID, uint32 time)
{
    if (apply)
    {
        if (HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
            return;

        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);

        GetMotionMaster()->MovementExpired(false);
        CastStop(GetObjectGuid() == casterGuid ? spellID : 0);

        Unit* caster = IsInWorld() ?  GetMap()->GetUnit(casterGuid) : NULL;

        GetMotionMaster()->MoveFleeing(caster, time);       // caster==NULL processed in MoveFleeing
    }
    else
    {
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);

        GetMotionMaster()->MovementExpired(false);

        if (GetTypeId() != TYPEID_PLAYER && isAlive())
        {
            Creature* c = ((Creature*)this);
            // restore appropriate movement generator
            if (getVictim())
                GetMotionMaster()->MoveChase(getVictim());
            else
                GetMotionMaster()->Initialize();

            // attack caster if can
            if (Unit* caster = IsInWorld() ? GetMap()->GetUnit(casterGuid) : NULL)
                if (c->AI())
                    c->AI()->AttackedBy(caster);
        }
    }

    if (GetTypeId() == TYPEID_PLAYER && !GetVehicle())
        ((Player*)this)->SetClientControl(this, !apply);
}

void Unit::SetConfused(bool apply, ObjectGuid casterGuid, uint32 spellID)
{
    if (apply)
    {
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);

        CastStop(GetObjectGuid() == casterGuid ? spellID : 0);

        GetMotionMaster()->MoveConfused();
    }
    else
    {
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);

        GetMotionMaster()->MovementExpired(false);

        if (GetTypeId() != TYPEID_PLAYER && isAlive())
        {
            // restore appropriate movement generator
            if(getVictim())
                GetMotionMaster()->MoveChase(getVictim());
            else
                GetMotionMaster()->Initialize();
        }
    }

    if(GetTypeId() == TYPEID_PLAYER && !GetVehicle())
        ((Player*)this)->SetClientControl(this, !apply);
}

void Unit::SetFeignDeath(bool apply, ObjectGuid casterGuid, uint32 /*spellID*/)
{
    if (apply)
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 9);
        data<<GetGUID();
        data<<uint8(0);
        SendMessageToSet(&data,true);
        */

        if (GetTypeId() != TYPEID_PLAYER)
            StopMoving();
        else
            ((Player*)this)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);

                                                            // blizz like 2.0.x
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                                                            // blizz like 2.0.x
        SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                                                            // blizz like 2.0.x
        SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        addUnitState(UNIT_STAT_DIED);
        CombatStop();
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // prevent interrupt message
        if (casterGuid == GetObjectGuid())
            FinishSpell(CURRENT_GENERIC_SPELL,false);
        InterruptNonMeleeSpells(true);
        getHostileRefManager().deleteReferences();
    }
    else
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 9);
        data<<GetGUID();
        data<<uint8(1);
        SendMessageToSet(&data,true);
        */
                                                            // blizz like 2.0.x
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                                                            // blizz like 2.0.x
        RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                                                            // blizz like 2.0.x
        RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        clearUnitState(UNIT_STAT_DIED);

        if (GetTypeId() != TYPEID_PLAYER && isAlive())
        {
            // restore appropriate movement generator
            if(getVictim())
                GetMotionMaster()->MoveChase(getVictim());
            else
                GetMotionMaster()->Initialize();
        }

    }
}

bool Unit::IsSitState() const
{
    uint8 s = getStandState();
    return
        s == UNIT_STAND_STATE_SIT_CHAIR        || s == UNIT_STAND_STATE_SIT_LOW_CHAIR  ||
        s == UNIT_STAND_STATE_SIT_MEDIUM_CHAIR || s == UNIT_STAND_STATE_SIT_HIGH_CHAIR ||
        s == UNIT_STAND_STATE_SIT;
}

bool Unit::IsStandState() const
{
    uint8 s = getStandState();
    return !IsSitState() && s != UNIT_STAND_STATE_SLEEP && s != UNIT_STAND_STATE_KNEEL;
}

void Unit::SetStandState(uint8 state)
{
    SetByteValue(UNIT_FIELD_BYTES_1, 0, state);

    if (IsStandState())
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_SEATED);

    if(GetTypeId()==TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_STANDSTATE_UPDATE, 1);
        data << (uint8)state;
        ((Player*)this)->GetSession()->SendPacket(&data);
    }
}

bool Unit::IsPolymorphed() const
{
    return GetSpellSpecific(getTransForm())==SPELL_MAGE_POLYMORPH;
}

void Unit::SetDisplayId(uint32 modelId)
{
    SetUInt32Value(UNIT_FIELD_DISPLAYID, modelId);

    UpdateModelData();

    if(GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(!pet->isControlled())
            return;
        Unit *owner = GetOwner();
        if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
            ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MODEL_ID);
    }
}

void Unit::UpdateModelData()
{
    if (CreatureModelInfo const* modelInfo = sObjectMgr.GetCreatureModelInfo(GetDisplayId()))
    {
        // we expect values in database to be relative to scale = 1.0
        SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, GetObjectScale() * modelInfo->bounding_radius);

        // never actually update combat_reach for player, it's always the same. Below player case is for initialization
        if (GetTypeId() == TYPEID_PLAYER)
            SetFloatValue(UNIT_FIELD_COMBATREACH, 1.5f);
        else
            SetFloatValue(UNIT_FIELD_COMBATREACH, GetObjectScale() * modelInfo->combat_reach);
    }
}

void Unit::ClearComboPointHolders()
{
    while(!m_ComboPointHolders.empty())
    {
        ObjectGuid guid = *m_ComboPointHolders.begin();

        Unit* owner = ObjectAccessor::GetUnit(*this, guid);
        if (owner && owner->GetComboTargetGuid() == GetObjectGuid())// recheck for safe
            owner->ClearComboPoints();                        // remove also guid from m_ComboPointHolders;
        else
            m_ComboPointHolders.erase(guid);             // or remove manually
    }
}

void Unit::AddComboPoints(Unit* target, int8 count)
{
    if (!count)
        return;

    // without combo points lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    if(target->GetObjectGuid() == m_comboTargetGuid)
    {
        m_comboPoints += count;
    }
    else
    {
        if (!m_comboTargetGuid.IsEmpty())
            if(Unit* target2 = ObjectAccessor::GetUnit(*this, m_comboTargetGuid))
                target2->RemoveComboPointHolder(GetObjectGuid());

        m_comboTargetGuid = target->GetObjectGuid();
        m_comboPoints = count;

        target->AddComboPointHolder(GetObjectGuid());
    }

    if (m_comboPoints > 5) m_comboPoints = 5;
    if (m_comboPoints < 0) m_comboPoints = 0;

    if (GetObjectGuid().IsPlayer())
        ((Player*)this)->SendComboPoints(m_comboTargetGuid, m_comboPoints);
    else if ((GetObjectGuid().IsPet() || GetObjectGuid().IsVehicle()) && GetCharmerOrOwner() && GetCharmerOrOwner()->GetObjectGuid().IsPlayer())
        ((Player*)GetCharmerOrOwner())->SendPetComboPoints(this,m_comboTargetGuid, m_comboPoints);
}

void Unit::ClearComboPoints()
{
    if (m_comboTargetGuid.IsEmpty())
        return;

    // without combopoints lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    m_comboPoints = 0;

    if (GetObjectGuid().IsPlayer())
        ((Player*)this)->SendComboPoints(m_comboTargetGuid, m_comboPoints);
    else if ((GetObjectGuid().IsPet() || GetObjectGuid().IsVehicle()) && GetCharmerOrOwner() && GetCharmerOrOwner()->GetObjectGuid().IsPlayer())
        ((Player*)GetCharmerOrOwner())->SendPetComboPoints(this,m_comboTargetGuid, m_comboPoints);

    if(Unit* target = ObjectAccessor::GetUnit(*this,m_comboTargetGuid))
        target->RemoveComboPointHolder(GetObjectGuid());

    m_comboTargetGuid.Clear();
}

void Unit::ClearAllReactives()
{
    for(int i=0; i < MAX_REACTIVE; ++i)
        m_reactiveTimer[i] = 0;

    if (HasAuraState( AURA_STATE_DEFENSE))
        ModifyAuraState(AURA_STATE_DEFENSE, false);
    if (getClass() == CLASS_HUNTER && HasAuraState( AURA_STATE_HUNTER_PARRY))
        ModifyAuraState(AURA_STATE_HUNTER_PARRY, false);
    if(getClass() == CLASS_WARRIOR && GetTypeId() == TYPEID_PLAYER)
        ClearComboPoints();
}

void Unit::UpdateReactives( uint32 p_time )
{
    for(int i = 0; i < MAX_REACTIVE; ++i)
    {
        ReactiveType reactive = ReactiveType(i);

        if(!m_reactiveTimer[reactive])
            continue;

        if ( m_reactiveTimer[reactive] <= p_time)
        {
            m_reactiveTimer[reactive] = 0;

            switch ( reactive )
            {
                case REACTIVE_DEFENSE:
                    if (HasAuraState(AURA_STATE_DEFENSE))
                        ModifyAuraState(AURA_STATE_DEFENSE, false);
                    break;
                case REACTIVE_HUNTER_PARRY:
                    if ( getClass() == CLASS_HUNTER && HasAuraState(AURA_STATE_HUNTER_PARRY))
                        ModifyAuraState(AURA_STATE_HUNTER_PARRY, false);
                    break;
                case REACTIVE_OVERPOWER:
                    if(getClass() == CLASS_WARRIOR && GetTypeId() == TYPEID_PLAYER)
                        ClearComboPoints();
                    break;
                default:
                    break;
            }
        }
        else
        {
            m_reactiveTimer[reactive] -= p_time;
        }
    }
}

Unit* Unit::SelectRandomUnfriendlyTarget(Unit* except /*= NULL*/, float radius /*= ATTACK_DISTANCE*/) const
{
    std::list<Unit *> targets;

    MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck u_check(this, this, radius);
    MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck> searcher(targets, u_check);
    Cell::VisitAllObjects(this, searcher, radius);

    // remove current target
    if(except)
        targets.remove(except);

    // remove not LoS targets
    for(std::list<Unit *>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        if(!IsWithinLOSInMap(*tIter))
        {
            std::list<Unit *>::iterator tIter2 = tIter;
            ++tIter;
            targets.erase(tIter2);
        }
        else
            ++tIter;
    }

    // no appropriate targets
    if(targets.empty())
        return NULL;

    // select random
    uint32 rIdx = urand(0,targets.size()-1);
    std::list<Unit *>::const_iterator tcIter = targets.begin();
    for(uint32 i = 0; i < rIdx; ++i)
        ++tcIter;

    return *tcIter;
}

Unit* Unit::SelectRandomFriendlyTarget(Unit* except /*= NULL*/, float radius /*= ATTACK_DISTANCE*/) const
{
    std::list<Unit *> targets;

    MaNGOS::AnyFriendlyUnitInObjectRangeCheck u_check(this, radius);
    MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck> searcher(targets, u_check);

    Cell::VisitAllObjects(this, searcher, radius);
    // remove current target
    if(except)
        targets.remove(except);

    // remove not LoS targets
    for(std::list<Unit *>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        if(!IsWithinLOSInMap(*tIter))
        {
            std::list<Unit *>::iterator tIter2 = tIter;
            ++tIter;
            targets.erase(tIter2);
        }
        else
            ++tIter;
    }

    // no appropriate targets
    if(targets.empty())
        return NULL;

    // select random
    uint32 rIdx = urand(0,targets.size()-1);
    std::list<Unit *>::const_iterator tcIter = targets.begin();
    for(uint32 i = 0; i < rIdx; ++i)
        ++tcIter;

    return *tcIter;
}

bool Unit::hasNegativeAuraWithInterruptFlag(uint32 flag)
{
    for (SpellAuraHolderMap::const_iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end(); ++iter)
    {
        if (!iter->second->IsPositive() && iter->second->GetSpellProto()->AuraInterruptFlags & flag)
            return true;
    }
    return false;
}

void Unit::ApplyAttackTimePercentMod( WeaponAttackType att,float val, bool apply )
{
    if(val > 0)
    {
        ApplyPercentModFloatVar(m_modAttackSpeedPct[att], val, !apply);
        ApplyPercentModFloatValue(UNIT_FIELD_BASEATTACKTIME+att,val,!apply);
    }
    else
    {
        ApplyPercentModFloatVar(m_modAttackSpeedPct[att], -val, apply);
        ApplyPercentModFloatValue(UNIT_FIELD_BASEATTACKTIME+att,-val,apply);
    }
}

void Unit::ApplyCastTimePercentMod(float val, bool apply )
{
    if(val > 0)
        ApplyPercentModFloatValue(UNIT_MOD_CAST_SPEED,val,!apply);
    else
        ApplyPercentModFloatValue(UNIT_MOD_CAST_SPEED,-val,apply);
}

void Unit::UpdateAuraForGroup(uint8 slot)
{
    if(GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = (Player*)this;
        if(player->GetGroup())
        {
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_AURAS);
            player->SetAuraUpdateMask(slot);
        }
    }
    else if(GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if(pet->isControlled())
        {
            Unit *owner = GetOwner();
            if(owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
            {
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_AURAS);
                pet->SetAuraUpdateMask(slot);
            }
        }
    }
}

float Unit::GetAPMultiplier(WeaponAttackType attType, bool normalized)
{
    if (!normalized || GetTypeId() != TYPEID_PLAYER)
        return float(GetAttackTime(attType))/1000.0f;

    Item *Weapon = ((Player*)this)->GetWeaponForAttack(attType, true, false);
    if (!Weapon)
        return 2.4f;                                         // fist attack

    switch (Weapon->GetProto()->InventoryType)
    {
        case INVTYPE_2HWEAPON:
            return 3.3f;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
            return 2.8f;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        default:
            return Weapon->GetProto()->SubClass==ITEM_SUBCLASS_WEAPON_DAGGER ? 1.7f : 2.4f;
    }
}

Aura* Unit::GetDummyAura( uint32 spell_id ) const
{
    Unit::AuraList const& mDummy = GetAurasByType(SPELL_AURA_DUMMY);
    for(Unit::AuraList::const_iterator itr = mDummy.begin(); itr != mDummy.end(); ++itr)
        if ((*itr)->GetId() == spell_id)
            return *itr;

    return NULL;
}

void Unit::SetContestedPvP(Player *attackedPlayer)
{
    Player* player = GetCharmerOrOwnerPlayerOrPlayerItself();

    if (!player || (attackedPlayer && (attackedPlayer == player || (player->duel && player->duel->opponent == attackedPlayer))))
        return;

    player->SetContestedPvPTimer(30000);

    if (!player->hasUnitState(UNIT_STAT_ATTACK_PLAYER))
    {
        player->addUnitState(UNIT_STAT_ATTACK_PLAYER);
        player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP);
        // call MoveInLineOfSight for nearby contested guards
        SetVisibility(GetVisibility());
    }

    if (!hasUnitState(UNIT_STAT_ATTACK_PLAYER))
    {
        addUnitState(UNIT_STAT_ATTACK_PLAYER);
        // call MoveInLineOfSight for nearby contested guards
        SetVisibility(GetVisibility());
    }
}

void Unit::AddPetAura(PetAura const* petSpell)
{
    m_petAuras.insert(petSpell);
    if(Pet* pet = GetPet())
    {
        GroupPetList m_groupPets = GetPets();
        if (!m_groupPets.empty())
        {
            for (GroupPetList::const_iterator itr = m_groupPets.begin(); itr != m_groupPets.end(); ++itr)
                if (Pet* _pet = GetMap()->GetPet(*itr))
                    _pet->CastPetAura(petSpell);
        }
    }

}

void Unit::RemovePetAura(PetAura const* petSpell)
{
    m_petAuras.erase(petSpell);
    if(Pet* pet = GetPet())
    {
        GroupPetList m_groupPets = GetPets();
        if (!m_groupPets.empty())
        {
            for (GroupPetList::const_iterator itr = m_groupPets.begin(); itr != m_groupPets.end(); ++itr)
                if (Pet* _pet = GetMap()->GetPet(*itr))
                    _pet->RemoveAurasDueToSpell(petSpell->GetAura(_pet->GetEntry()));
        }
    }
}

void Unit::RemoveAurasAtMechanicImmunity(uint32 mechMask, uint32 exceptSpellId, bool non_positive /*= false*/)
{
    Unit::SpellAuraHolderMap& auras = GetSpellAuraHolderMap();
    for(Unit::SpellAuraHolderMap::iterator iter = auras.begin(); iter != auras.end();)
    {
        SpellEntry const *spell = iter->second->GetSpellProto();
        if (spell->Id == exceptSpellId)
            ++iter;
        else if (non_positive && iter->second->IsPositive())
            ++iter;
        else if (spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)
            ++iter;
        else if (iter->second->HasMechanicMask(mechMask))
        {
            RemoveAurasDueToSpell(spell->Id);

            if(auras.empty())
                break;
            else
                iter = auras.begin();
         }
        else
            ++iter;
    }
}

void Unit::RemoveAurasBySpellMechanic(uint32 mechMask)
{
    Unit::SpellAuraHolderMap& auras = GetSpellAuraHolderMap();
    for(Unit::SpellAuraHolderMap::iterator iter = auras.begin(); iter != auras.end();)
    {
        SpellEntry const *spell = iter->second->GetSpellProto();

        if (!iter->second->IsPositive())
            ++iter;

        else if (spell->Mechanic & mechMask)
        {
            RemoveAurasDueToSpell(spell->Id);
            if(auras.empty())
                break;
            else
                iter = auras.begin();
        }
        else
            ++iter;
    }
}

void Unit::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    if(newPhaseMask==GetPhaseMask())
        return;

    if(IsInWorld())
        RemoveNotOwnSingleTargetAuras(newPhaseMask);        // we can lost access to caster or target

    WorldObject::SetPhaseMask(newPhaseMask,update);

    if(IsInWorld())
        if(Pet* pet = GetPet())
            pet->SetPhaseMask(newPhaseMask,true);
}

void Unit::NearTeleportTo( float x, float y, float z, float orientation, bool casting /*= false*/ )
{
    if(GetTypeId() == TYPEID_PLAYER)
        ((Player*)this)->TeleportTo(GetMapId(), x, y, z, orientation, TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (casting ? TELE_TO_SPELL : 0));
    else
    {
        ExitVehicle();
        Creature* c = (Creature*)this;
        // Creature relocation acts like instant movement generator, so current generator expects interrupt/reset calls to react properly
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Interrupt(*c);

        SetPosition(x, y, z, orientation, true);

        SendHeartBeat(false);

        // finished relocation, movegen can different from top before creature relocation,
        // but apply Reset expected to be safe in any case
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Reset(*c);
    }
}

void Unit::MonsterMove(float x, float y, float z, uint32 transitTime)
{
    SplineFlags flags = GetTypeId() == TYPEID_PLAYER ? SPLINEFLAG_WALKMODE : ((Creature*)this)->GetSplineFlags();
    SendMonsterMove(x, y, z, SPLINETYPE_NORMAL, flags, transitTime);

    if (GetTypeId() != TYPEID_PLAYER)
    {
        Creature* c = (Creature*)this;
        // Creature relocation acts like instant movement generator, so current generator expects interrupt/reset calls to react properly
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Interrupt(*c);

        GetMap()->CreatureRelocation((Creature*)this, x, y, z, 0.0f);

        // finished relocation, movegen can different from top before creature relocation,
        // but apply Reset expected to be safe in any case
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Reset(*c);
    }
}

void Unit::MonsterMoveWithSpeed(float x, float y, float z, uint32 transitTime)
{
    SendMonsterMoveWithSpeed(x, y, z, transitTime );

    if (GetTypeId() != TYPEID_PLAYER)
    {
        Creature* c = (Creature*)this;
        // Creature relocation acts like instant movement generator, so current generator expects interrupt/reset calls to react properly
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Interrupt(*c);

        GetMap()->CreatureRelocation((Creature*)this, x, y, z, 0.0f);

        // finished relocation, movegen can different from top before creature relocation,
        // but apply Reset expected to be safe in any case
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Reset(*c);
    }
}

void Unit::MonsterJump(float x, float y, float z, float o, uint32 transitTime, uint32 verticalSpeed)
{
    SendMonsterMove(x, y, z, SPLINETYPE_NORMAL, SplineFlags(SPLINEFLAG_TRAJECTORY | SPLINEFLAG_WALKMODE), transitTime, NULL, double(verticalSpeed));

    if (GetTypeId() != TYPEID_PLAYER)
    {
        Creature* c = (Creature*)this;
        // Creature relocation acts like instant movement generator, so current generator expects interrupt/reset calls to react properly
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Interrupt(*c);

        GetMap()->CreatureRelocation((Creature*)this, x, y, z, o);

        // finished relocation, movegen can different from top before creature relocation,
        // but apply Reset expected to be safe in any case
        if (!c->GetMotionMaster()->empty())
            if (MovementGenerator *movgen = c->GetMotionMaster()->top())
                movgen->Reset(*c);
    }
}

struct SetPvPHelper
{
    explicit SetPvPHelper(bool _state) : state(_state) {}
    void operator()(Unit* unit) const { unit->SetPvP(state); }
    bool state;
};

bool Unit::CreateVehicleKit(uint32 vehicleId)
{
    VehicleEntry const *vehicleInfo = sVehicleStore.LookupEntry(vehicleId);

    if (!vehicleInfo)
        return false;

    m_pVehicleKit = new VehicleKit(this, vehicleInfo);
    m_updateFlag |= UPDATEFLAG_VEHICLE;
    return true;
}

void Unit::RemoveVehicleKit()
{
    if (!m_pVehicleKit)
        return;

    m_pVehicleKit->RemoveAllPassengers();

    delete m_pVehicleKit;
    m_pVehicleKit = NULL;

    m_updateFlag &= ~UPDATEFLAG_VEHICLE;
    RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
}

void Unit::ChangeSeat(int8 seatId, bool next)
{
    if (!m_pVehicle)
        return;

    if (seatId < 0)
    {
        seatId = m_pVehicle->GetNextEmptySeat(m_movementInfo.GetTransportSeat(), next);
        if (seatId < 0)
            return;
    }
    else if (seatId == m_movementInfo.GetTransportSeat() || !m_pVehicle->HasEmptySeat(seatId))
        return;

    m_pVehicle->RemovePassenger(this);
    m_pVehicle->AddPassenger(this, seatId);
}

void Unit::EnterVehicle(VehicleKit *vehicle, int8 seatId)
{
    if (!isAlive() || GetVehicleKit() == vehicle)
        return;

    if (m_pVehicle)
    {
        if (m_pVehicle == vehicle)
        {
            if (seatId >= 0)
                ChangeSeat(seatId);

            return;
        }
        else
            ExitVehicle();
    }

    InterruptNonMeleeSpells(false);
    RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);

    if (!vehicle->AddPassenger(this, seatId))
        return;

    m_pVehicle = vehicle;

    if (Pet *pet = GetPet())
        pet->Unsummon(PET_SAVE_AS_CURRENT,this);

    if (GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = (Player*)this;

        if (BattleGround *bg = player->GetBattleGround())
            bg->EventPlayerDroppedFlag(player);

        WorldPacket data(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA);
        player->GetSession()->SendPacket(&data);

        data.Initialize(SMSG_BREAK_TARGET, 8);
        data << vehicle->GetBase()->GetPackGUID();
        player->GetSession()->SendPacket(&data);
    }

    if (Transport* pTransport = GetTransport())
    {
        if (GetTypeId() == TYPEID_PLAYER)
            pTransport->RemovePassenger((Player*)this);

        SetTransport(NULL);
    }
}

void Unit::ExitVehicle()
{
    if(!m_pVehicle)
        return;

    m_pVehicle->RemovePassenger(this);
    m_pVehicle = NULL;

    if (GetTypeId() == TYPEID_PLAYER)
        ((Player*)this)->ResummonPetTemporaryUnSummonedIfAny();

    float x = GetPositionX();
    float y = GetPositionY();
    float z = GetPositionZ() + 2.0f;
    GetClosePoint(x, y, z, 2.0f);
    UpdateAllowedPositionZ(x, y, z);
    SendMonsterMove(x, y, z + 0.5f, SPLINETYPE_NORMAL, SPLINEFLAG_WALKMODE, 0);
}

void Unit::SetPvP( bool state )
{
    if(state)
        SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);
    else
        RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);

    CallForAllControlledUnits(SetPvPHelper(state), CONTROLLED_PET|CONTROLLED_TOTEMS|CONTROLLED_GUARDIANS|CONTROLLED_CHARM);
}

struct SetFFAPvPHelper
{
    explicit SetFFAPvPHelper(bool _state) : state(_state) {}
    void operator()(Unit* unit) const { unit->SetFFAPvP(state); }
    bool state;
};

void Unit::SetFFAPvP( bool state )
{
    if(state)
        SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);
    else
        RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);

    CallForAllControlledUnits(SetFFAPvPHelper(state), CONTROLLED_PET|CONTROLLED_TOTEMS|CONTROLLED_GUARDIANS|CONTROLLED_CHARM);
}

void Unit::KnockBackFrom(Unit* target, float horizontalSpeed, float verticalSpeed)
{
    float angle = this == target ? GetOrientation() + M_PI_F : target->GetAngle(this);
    float vsin = sin(angle);
    float vcos = cos(angle);

    // Effect properly implemented only for players
    if(GetTypeId()==TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_MOVE_KNOCK_BACK, 8+4+4+4+4+4);
        data << GetPackGUID();
        data << uint32(0);                                  // Sequence
        data << float(vcos);                                // x direction
        data << float(vsin);                                // y direction
        data << float(horizontalSpeed);                     // Horizontal speed
        data << float(-verticalSpeed);                      // Z Movement speed (vertical)
        ((Player*)this)->GetSession()->SendPacket(&data);
    }
    else
    {
        float dh = verticalSpeed*verticalSpeed / (2*19.23f); // maximum parabola height
        float time = (verticalSpeed) ? sqrtf(dh/(0.124976 * verticalSpeed)) : 0.0f;  //full move time in seconds
 
        float dis = time * horizontalSpeed;

        float ox, oy, oz;
        GetPosition(ox, oy, oz);

        float fx = ox + dis * vcos;
        float fy = oy + dis * vsin;
        float fz = oz;

        float fx2, fy2, fz2;                                // getObjectHitPos overwrite last args in any result case
        if(VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(), ox,oy,oz+0.5f, fx,fy,oz+0.5f,fx2,fy2,fz2, -0.5f))
        {
            fx = fx2;
            fy = fy2;
            fz = fz2;
        }

        UpdateAllowedPositionZ(fx, fy, fz);

        GetMap()->CreatureRelocation((Creature*)this, fx, fy, fz, GetOrientation());//it's a hack, need motion master support
        SendMonsterMoveJump(fx, fy, fz, verticalSpeed, SPLINEFLAG_WALKMODE, uint32(time * 1000.0f));
    }
}

void Unit::KnockBackPlayerWithAngle(float angle, float horizontalSpeed, float verticalSpeed)
{
    float vsin = sin(angle);
    float vcos = cos(angle);

    // Effect propertly implemented only for players
    if(GetTypeId()==TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_MOVE_KNOCK_BACK, 8+4+4+4+4+4);
        data << GetPackGUID();
        data << uint32(0);                                  // Sequence
        data << float(vcos);                                // x direction
        data << float(vsin);                                // y direction
        data << float(horizontalSpeed);                     // Horizontal speed
        data << float(-verticalSpeed);                      // Z Movement speed (vertical)
        ((Player*)this)->GetSession()->SendPacket(&data);
    }
    else
        sLog.outError("KnockBackPlayer: Target of KnockBackPlayer must be player!");
}

float Unit::GetCombatRatingReduction(CombatRating cr) const
{
    if (GetTypeId() == TYPEID_PLAYER)
        return ((Player const*)this)->GetRatingBonusValue(cr);
    else if (((Creature const*)this)->IsPet())
    {
        // Player's pet get 100% resilience from owner
        if (Unit* owner = GetOwner())
            if(owner->GetTypeId() == TYPEID_PLAYER)
                return ((Player*)owner)->GetRatingBonusValue(cr);
    }

    return 0.0f;
}

uint32 Unit::GetCombatRatingDamageReduction(CombatRating cr, float rate, float cap, uint32 damage) const
{
    float percent = GetCombatRatingReduction(cr) * rate;
    if (percent > cap)
        percent = cap;
    return uint32 (percent * damage / 100.0f);
}

void Unit::SendThreatUpdate()
{
    ThreatList const& tlist = getThreatManager().getThreatList();
    if (uint32 count = tlist.size())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "WORLD: Send SMSG_THREAT_UPDATE Message");
        WorldPacket data(SMSG_THREAT_UPDATE, 8 + count * 8);
        data << GetPackGUID();
        data << uint32(count);
        for (ThreatList::const_iterator itr = tlist.begin(); itr != tlist.end(); ++itr)
        {
            data.appendPackGUID((*itr)->getUnitGuid());
            data << uint32((*itr)->getThreat());
        }
        SendMessageToSet(&data, false);
    }
}

void Unit::SendHighestThreatUpdate(HostileReference* pHostilReference)
{
    ThreatList const& tlist = getThreatManager().getThreatList();
    if (uint32 count = tlist.size())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "WORLD: Send SMSG_HIGHEST_THREAT_UPDATE Message");
        WorldPacket data(SMSG_HIGHEST_THREAT_UPDATE, 8 + 8 + count * 8);
        data << GetPackGUID();
        data.appendPackGUID(pHostilReference->getUnitGuid());
        data << uint32(count);
        for (ThreatList::const_iterator itr = tlist.begin(); itr != tlist.end(); ++itr)
        {
            data.appendPackGUID((*itr)->getUnitGuid());
            data << uint32((*itr)->getThreat());
        }
        SendMessageToSet(&data, false);
    }
}

void Unit::SendThreatClear()
{
    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "WORLD: Send SMSG_THREAT_CLEAR Message");
    WorldPacket data(SMSG_THREAT_CLEAR, 8);
    data << GetPackGUID();
    SendMessageToSet(&data, false);
}

void Unit::SendThreatRemove(HostileReference* pHostileReference)
{
    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "WORLD: Send SMSG_THREAT_REMOVE Message");
    WorldPacket data(SMSG_THREAT_REMOVE, 8 + 8);
    data << GetPackGUID();
    data.appendPackGUID(pHostileReference->getUnitGuid());
    SendMessageToSet(&data, false);
}

struct StopAttackFactionHelper
{
    explicit StopAttackFactionHelper(uint32 _faction_id) : faction_id(_faction_id) {}
    void operator()(Unit* unit) const { unit->StopAttackFaction(faction_id); }
    uint32 faction_id;
};

void Unit::StopAttackFaction(uint32 faction_id)
{
    if (Unit* victim = getVictim())
    {
        if (victim->getFactionTemplateEntry()->faction==faction_id)
        {
            AttackStop();
            if (IsNonMeleeSpellCasted(false))
                InterruptNonMeleeSpells(false);

            // melee and ranged forced attack cancel
            if (GetTypeId() == TYPEID_PLAYER)
                ((Player*)this)->SendAttackSwingCancelAttack();
        }
    }

    AttackerSet const& attackers = getAttackers();
    for(AttackerSet::const_iterator itr = attackers.begin(); itr != attackers.end();)
    {
        if ((*itr)->getFactionTemplateEntry()->faction==faction_id)
        {
            (*itr)->AttackStop();
            itr = attackers.begin();
        }
        else
            ++itr;
    }

    getHostileRefManager().deleteReferencesForFaction(faction_id);

    CallForAllControlledUnits(StopAttackFactionHelper(faction_id), CONTROLLED_PET|CONTROLLED_GUARDIANS|CONTROLLED_CHARM);
}

bool Unit::IsIgnoreUnitState(SpellEntry const *spell, IgnoreUnitState ignoreState)
{
    Unit::AuraList const& stateAuras = GetAurasByType(SPELL_AURA_IGNORE_UNIT_STATE);
    for(Unit::AuraList::const_iterator itr = stateAuras.begin(); itr != stateAuras.end(); ++itr)
    {
        if ((*itr)->GetModifier()->m_miscvalue == ignoreState)
        {
            // frozen state absent ignored for all spells
            if (ignoreState == IGNORE_UNIT_TARGET_NON_FROZEN)
                return true;

            if ((*itr)->isAffectedOnSpell(spell))
                return true;
        }
    }

    return false;
}

void Unit::CleanupDeletedAuras()
{
    for (SpellAuraHolderList::const_iterator iter = m_deletedHolders.begin(); iter != m_deletedHolders.end(); ++iter)
        delete *iter;
    m_deletedHolders.clear();

    // really delete auras "deleted" while processing its ApplyModify code
    for(AuraList::const_iterator itr = m_deletedAuras.begin(); itr != m_deletedAuras.end(); ++itr)
        delete *itr;
    m_deletedAuras.clear();
}

bool Unit::CheckAndIncreaseCastCounter()
{
    uint32 maxCasts = sWorld.getConfig(CONFIG_UINT32_MAX_SPELL_CASTS_IN_CHAIN);

    if (maxCasts && m_castCounter >= maxCasts)
        return false;

    ++m_castCounter;
    return true;
}

SpellAuraHolder* Unit::GetSpellAuraHolder (uint32 spellid, uint64 casterGUID)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellid);
    for (SpellAuraHolderMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        if (!casterGUID || iter->second->GetCasterGUID() == casterGUID)
            return iter->second;

    return NULL;
}

void Unit::_AddAura(uint32 spellID, uint32 duration)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry( spellID );

    if(spellInfo)
    {
        if (IsSpellAppliesAura(spellInfo, (1 << EFFECT_INDEX_0) | (1 << EFFECT_INDEX_1) | (1 << EFFECT_INDEX_2)) || IsSpellHaveEffect(spellInfo, SPELL_EFFECT_PERSISTENT_AREA_AURA))
        {
            SpellAuraHolder* holder = CreateSpellAuraHolder(spellInfo, this, this);

            for(uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                if (spellInfo->Effect[i] >= TOTAL_SPELL_EFFECTS)
                    continue;
                if( IsAreaAuraEffect(spellInfo->Effect[i])           ||
                    spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AURA  ||
                    spellInfo->Effect[i] == SPELL_EFFECT_PERSISTENT_AREA_AURA )
                {
                    Aura *aura = CreateAura(spellInfo, SpellEffectIndex(i), NULL, holder, this);
                    aura->SetAuraDuration(duration);
                    holder->AddAura(aura, SpellEffectIndex(i));
                    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Manually adding aura of spell %u, index %u, duration %u ms", spellID, i, duration);
                }
            }
            AddSpellAuraHolder(holder);
        }
    }
}

bool Unit::IsAllowedDamageInArea(Unit* pVictim) const
{
    // can damage self anywhere
    if (pVictim == this)
        return true;

    // non player controlled unit can damage anywhere
    if (!IsCharmerOrOwnerPlayerOrPlayerItself())
        return true;

    // can damage own pet anywhere
    if (pVictim->GetOwnerGuid() == GetObjectGuid())
        return true;

    // can damage non player controlled victim anywhere
    if (!pVictim->IsCharmerOrOwnerPlayerOrPlayerItself())
        return true;

    // can't damage player controlled unit by player controlled unit in sanctuary
    AreaTableEntry const* area = GetAreaEntryByAreaID(pVictim->GetAreaId());
    if (area && area->flags & AREA_FLAG_SANCTUARY)
        return false;

    return true;
}
