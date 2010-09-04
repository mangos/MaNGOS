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

#include "Common.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "CreatureAI.h"
#include "ScriptCalls.h"
#include "Util.h"

pAuraProcHandler AuraProcHandler[TOTAL_AURAS]=
{
    &Unit::HandleNULLProc,                                      //  0 SPELL_AURA_NONE
    &Unit::HandleNULLProc,                                      //  1 SPELL_AURA_BIND_SIGHT
    &Unit::HandleNULLProc,                                      //  2 SPELL_AURA_MOD_POSSESS
    &Unit::HandleNULLProc,                                      //  3 SPELL_AURA_PERIODIC_DAMAGE
    &Unit::HandleDummyAuraProc,                                 //  4 SPELL_AURA_DUMMY
    &Unit::HandleNULLProc,                                      //  5 SPELL_AURA_MOD_CONFUSE
    &Unit::HandleNULLProc,                                      //  6 SPELL_AURA_MOD_CHARM
    &Unit::HandleNULLProc,                                      //  7 SPELL_AURA_MOD_FEAR
    &Unit::HandleNULLProc,                                      //  8 SPELL_AURA_PERIODIC_HEAL
    &Unit::HandleNULLProc,                                      //  9 SPELL_AURA_MOD_ATTACKSPEED
    &Unit::HandleNULLProc,                                      // 10 SPELL_AURA_MOD_THREAT
    &Unit::HandleNULLProc,                                      // 11 SPELL_AURA_MOD_TAUNT
    &Unit::HandleNULLProc,                                      // 12 SPELL_AURA_MOD_STUN
    &Unit::HandleNULLProc,                                      // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &Unit::HandleNULLProc,                                      // 14 SPELL_AURA_MOD_DAMAGE_TAKEN
    &Unit::HandleNULLProc,                                      // 15 SPELL_AURA_DAMAGE_SHIELD
    &Unit::HandleNULLProc,                                      // 16 SPELL_AURA_MOD_STEALTH
    &Unit::HandleNULLProc,                                      // 17 SPELL_AURA_MOD_STEALTH_DETECT
    &Unit::HandleNULLProc,                                      // 18 SPELL_AURA_MOD_INVISIBILITY
    &Unit::HandleNULLProc,                                      // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &Unit::HandleNULLProc,                                      // 20 SPELL_AURA_OBS_MOD_HEALTH
    &Unit::HandleNULLProc,                                      // 21 SPELL_AURA_OBS_MOD_MANA
    &Unit::HandleNULLProc,                                      // 22 SPELL_AURA_MOD_RESISTANCE
    &Unit::HandleNULLProc,                                      // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL
    &Unit::HandleNULLProc,                                      // 24 SPELL_AURA_PERIODIC_ENERGIZE
    &Unit::HandleNULLProc,                                      // 25 SPELL_AURA_MOD_PACIFY
    &Unit::HandleNULLProc,                                      // 26 SPELL_AURA_MOD_ROOT
    &Unit::HandleNULLProc,                                      // 27 SPELL_AURA_MOD_SILENCE
    &Unit::HandleNULLProc,                                      // 28 SPELL_AURA_REFLECT_SPELLS
    &Unit::HandleNULLProc,                                      // 29 SPELL_AURA_MOD_STAT
    &Unit::HandleNULLProc,                                      // 30 SPELL_AURA_MOD_SKILL
    &Unit::HandleNULLProc,                                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &Unit::HandleNULLProc,                                      // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &Unit::HandleNULLProc,                                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &Unit::HandleNULLProc,                                      // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &Unit::HandleNULLProc,                                      // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &Unit::HandleNULLProc,                                      // 36 SPELL_AURA_MOD_SHAPESHIFT
    &Unit::HandleNULLProc,                                      // 37 SPELL_AURA_EFFECT_IMMUNITY
    &Unit::HandleNULLProc,                                      // 38 SPELL_AURA_STATE_IMMUNITY
    &Unit::HandleNULLProc,                                      // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &Unit::HandleNULLProc,                                      // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &Unit::HandleNULLProc,                                      // 41 SPELL_AURA_DISPEL_IMMUNITY
    &Unit::HandleProcTriggerSpellAuraProc,                      // 42 SPELL_AURA_PROC_TRIGGER_SPELL
    &Unit::HandleProcTriggerDamageAuraProc,                     // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE
    &Unit::HandleNULLProc,                                      // 44 SPELL_AURA_TRACK_CREATURES
    &Unit::HandleNULLProc,                                      // 45 SPELL_AURA_TRACK_RESOURCES
    &Unit::HandleNULLProc,                                      // 46 SPELL_AURA_46 (used in test spells 54054 and 54058, and spell 48050) (3.0.8a-3.2.2a)
    &Unit::HandleNULLProc,                                      // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &Unit::HandleNULLProc,                                      // 48 SPELL_AURA_48 spell Napalm (area damage spell with additional delayed damage effect)
    &Unit::HandleNULLProc,                                      // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &Unit::HandleNULLProc,                                      // 50 SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT
    &Unit::HandleNULLProc,                                      // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &Unit::HandleNULLProc,                                      // 52 SPELL_AURA_MOD_CRIT_PERCENT
    &Unit::HandleNULLProc,                                      // 53 SPELL_AURA_PERIODIC_LEECH
    &Unit::HandleNULLProc,                                      // 54 SPELL_AURA_MOD_HIT_CHANCE
    &Unit::HandleNULLProc,                                      // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &Unit::HandleNULLProc,                                      // 56 SPELL_AURA_TRANSFORM
    &Unit::HandleSpellCritChanceAuraProc,                       // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &Unit::HandleNULLProc,                                      // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &Unit::HandleNULLProc,                                      // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE
    &Unit::HandleNULLProc,                                      // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &Unit::HandleNULLProc,                                      // 61 SPELL_AURA_MOD_SCALE
    &Unit::HandleNULLProc,                                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
    &Unit::HandleNULLProc,                                      // 63 unused (3.0.8a-3.2.2a) old SPELL_AURA_PERIODIC_MANA_FUNNEL
    &Unit::HandleNULLProc,                                      // 64 SPELL_AURA_PERIODIC_MANA_LEECH
    &Unit::HandleModCastingSpeedNotStackAuraProc,               // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &Unit::HandleNULLProc,                                      // 66 SPELL_AURA_FEIGN_DEATH
    &Unit::HandleNULLProc,                                      // 67 SPELL_AURA_MOD_DISARM
    &Unit::HandleNULLProc,                                      // 68 SPELL_AURA_MOD_STALKED
    &Unit::HandleNULLProc,                                      // 69 SPELL_AURA_SCHOOL_ABSORB
    &Unit::HandleNULLProc,                                      // 70 SPELL_AURA_EXTRA_ATTACKS      Useless, used by only one spell 41560 that has only visual effect (3.2.2a)
    &Unit::HandleNULLProc,                                      // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &Unit::HandleModPowerCostSchoolAuraProc,                    // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &Unit::HandleModPowerCostSchoolAuraProc,                    // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &Unit::HandleReflectSpellsSchoolAuraProc,                   // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL
    &Unit::HandleNULLProc,                                      // 75 SPELL_AURA_MOD_LANGUAGE
    &Unit::HandleNULLProc,                                      // 76 SPELL_AURA_FAR_SIGHT
    &Unit::HandleMechanicImmuneResistanceAuraProc,              // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &Unit::HandleNULLProc,                                      // 78 SPELL_AURA_MOUNTED
    &Unit::HandleModDamagePercentDoneAuraProc,                  // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &Unit::HandleNULLProc,                                      // 80 SPELL_AURA_MOD_PERCENT_STAT
    &Unit::HandleNULLProc,                                      // 81 SPELL_AURA_SPLIT_DAMAGE_PCT
    &Unit::HandleNULLProc,                                      // 82 SPELL_AURA_WATER_BREATHING
    &Unit::HandleNULLProc,                                      // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &Unit::HandleNULLProc,                                      // 84 SPELL_AURA_MOD_REGEN
    &Unit::HandleCantTrigger,                                   // 85 SPELL_AURA_MOD_POWER_REGEN
    &Unit::HandleNULLProc,                                      // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &Unit::HandleNULLProc,                                      // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN
    &Unit::HandleNULLProc,                                      // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT
    &Unit::HandleNULLProc,                                      // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &Unit::HandleNULLProc,                                      // 90 unused (3.0.8a-3.2.2a) old SPELL_AURA_MOD_RESIST_CHANCE
    &Unit::HandleNULLProc,                                      // 91 SPELL_AURA_MOD_DETECT_RANGE
    &Unit::HandleNULLProc,                                      // 92 SPELL_AURA_PREVENTS_FLEEING
    &Unit::HandleNULLProc,                                      // 93 SPELL_AURA_MOD_UNATTACKABLE
    &Unit::HandleNULLProc,                                      // 94 SPELL_AURA_INTERRUPT_REGEN
    &Unit::HandleNULLProc,                                      // 95 SPELL_AURA_GHOST
    &Unit::HandleNULLProc,                                      // 96 SPELL_AURA_SPELL_MAGNET
    &Unit::HandleNULLProc,                                      // 97 SPELL_AURA_MANA_SHIELD
    &Unit::HandleNULLProc,                                      // 98 SPELL_AURA_MOD_SKILL_TALENT
    &Unit::HandleNULLProc,                                      // 99 SPELL_AURA_MOD_ATTACK_POWER
    &Unit::HandleNULLProc,                                      //100 SPELL_AURA_AURAS_VISIBLE obsolete 3.x? all player can see all auras now, but still have 2 spells including GM-spell (1852,2855)
    &Unit::HandleNULLProc,                                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &Unit::HandleNULLProc,                                      //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS
    &Unit::HandleNULLProc,                                      //103 SPELL_AURA_MOD_TOTAL_THREAT
    &Unit::HandleNULLProc,                                      //104 SPELL_AURA_WATER_WALK
    &Unit::HandleNULLProc,                                      //105 SPELL_AURA_FEATHER_FALL
    &Unit::HandleNULLProc,                                      //106 SPELL_AURA_HOVER
    &Unit::HandleNULLProc,                                      //107 SPELL_AURA_ADD_FLAT_MODIFIER
    &Unit::HandleAddPctModifierAuraProc,                        //108 SPELL_AURA_ADD_PCT_MODIFIER
    &Unit::HandleNULLProc,                                      //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &Unit::HandleNULLProc,                                      //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
    &Unit::HandleNULLProc,                                      //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER
    &Unit::HandleOverrideClassScriptAuraProc,                   //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS
    &Unit::HandleNULLProc,                                      //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN
    &Unit::HandleNULLProc,                                      //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT
    &Unit::HandleNULLProc,                                      //115 SPELL_AURA_MOD_HEALING
    &Unit::HandleNULLProc,                                      //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT
    &Unit::HandleMechanicImmuneResistanceAuraProc,              //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE
    &Unit::HandleNULLProc,                                      //118 SPELL_AURA_MOD_HEALING_PCT
    &Unit::HandleNULLProc,                                      //119 unused (3.0.8a-3.2.2a) old SPELL_AURA_SHARE_PET_TRACKING
    &Unit::HandleNULLProc,                                      //120 SPELL_AURA_UNTRACKABLE
    &Unit::HandleNULLProc,                                      //121 SPELL_AURA_EMPATHY
    &Unit::HandleNULLProc,                                      //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &Unit::HandleNULLProc,                                      //123 SPELL_AURA_MOD_TARGET_RESISTANCE
    &Unit::HandleNULLProc,                                      //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &Unit::HandleNULLProc,                                      //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN
    &Unit::HandleNULLProc,                                      //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT
    &Unit::HandleNULLProc,                                      //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS
    &Unit::HandleNULLProc,                                      //128 SPELL_AURA_MOD_POSSESS_PET
    &Unit::HandleNULLProc,                                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &Unit::HandleNULLProc,                                      //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &Unit::HandleNULLProc,                                      //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS
    &Unit::HandleNULLProc,                                      //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &Unit::HandleNULLProc,                                      //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &Unit::HandleNULLProc,                                      //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &Unit::HandleNULLProc,                                      //135 SPELL_AURA_MOD_HEALING_DONE
    &Unit::HandleNULLProc,                                      //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT
    &Unit::HandleNULLProc,                                      //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &Unit::HandleHasteAuraProc,                                 //138 SPELL_AURA_MOD_HASTE
    &Unit::HandleNULLProc,                                      //139 SPELL_AURA_FORCE_REACTION
    &Unit::HandleNULLProc,                                      //140 SPELL_AURA_MOD_RANGED_HASTE
    &Unit::HandleNULLProc,                                      //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
    &Unit::HandleNULLProc,                                      //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &Unit::HandleNULLProc,                                      //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &Unit::HandleNULLProc,                                      //144 SPELL_AURA_SAFE_FALL
    &Unit::HandleNULLProc,                                      //145 SPELL_AURA_MOD_PET_TALENT_POINTS
    &Unit::HandleNULLProc,                                      //146 SPELL_AURA_ALLOW_TAME_PET_TYPE
    &Unit::HandleNULLProc,                                      //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK
    &Unit::HandleNULLProc,                                      //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &Unit::HandleCantTrigger,                                   //149 SPELL_AURA_REDUCE_PUSHBACK
    &Unit::HandleNULLProc,                                      //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &Unit::HandleNULLProc,                                      //151 SPELL_AURA_TRACK_STEALTHED
    &Unit::HandleNULLProc,                                      //152 SPELL_AURA_MOD_DETECTED_RANGE
    &Unit::HandleNULLProc,                                      //153 SPELL_AURA_SPLIT_DAMAGE_FLAT
    &Unit::HandleNULLProc,                                      //154 SPELL_AURA_MOD_STEALTH_LEVEL
    &Unit::HandleNULLProc,                                      //155 SPELL_AURA_MOD_WATER_BREATHING
    &Unit::HandleNULLProc,                                      //156 SPELL_AURA_MOD_REPUTATION_GAIN
    &Unit::HandleNULLProc,                                      //157 SPELL_AURA_PET_DAMAGE_MULTI (single test like spell 20782, also single for 214 aura)
    &Unit::HandleNULLProc,                                      //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &Unit::HandleNULLProc,                                      //159 SPELL_AURA_NO_PVP_CREDIT
    &Unit::HandleNULLProc,                                      //160 SPELL_AURA_MOD_AOE_AVOIDANCE
    &Unit::HandleNULLProc,                                      //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT
    &Unit::HandleNULLProc,                                      //162 SPELL_AURA_POWER_BURN_MANA
    &Unit::HandleNULLProc,                                      //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS
    &Unit::HandleNULLProc,                                      //164 unused (3.0.8a-3.2.2a), only one test spell 10654
    &Unit::HandleNULLProc,                                      //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS
    &Unit::HandleNULLProc,                                      //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &Unit::HandleNULLProc,                                      //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &Unit::HandleNULLProc,                                      //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS
    &Unit::HandleNULLProc,                                      //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS
    &Unit::HandleNULLProc,                                      //170 SPELL_AURA_DETECT_AMORE       different spells that ignore transformation effects
    &Unit::HandleNULLProc,                                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &Unit::HandleNULLProc,                                      //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &Unit::HandleNULLProc,                                      //173 unused (3.0.8a-3.2.2a) no spells, old SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &Unit::HandleNULLProc,                                      //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT
    &Unit::HandleNULLProc,                                      //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT
    &Unit::HandleNULLProc,                                      //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &Unit::HandleNULLProc,                                      //177 SPELL_AURA_AOE_CHARM (22 spells)
    &Unit::HandleNULLProc,                                      //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE
    &Unit::HandleNULLProc,                                      //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE
    &Unit::HandleNULLProc,                                      //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS
    &Unit::HandleNULLProc,                                      //181 unused (3.0.8a-3.2.2a) old SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS
    &Unit::HandleNULLProc,                                      //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &Unit::HandleNULLProc,                                      //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746
    &Unit::HandleNULLProc,                                      //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE
    &Unit::HandleNULLProc,                                      //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE
    &Unit::HandleNULLProc,                                      //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE
    &Unit::HandleNULLProc,                                      //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE
    &Unit::HandleNULLProc,                                      //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE
    &Unit::HandleNULLProc,                                      //189 SPELL_AURA_MOD_RATING
    &Unit::HandleNULLProc,                                      //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN
    &Unit::HandleNULLProc,                                      //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
    &Unit::HandleNULLProc,                                      //192 SPELL_AURA_HASTE_MELEE
    &Unit::HandleNULLProc,                                      //193 SPELL_AURA_HASTE_ALL (in fact combat (any type attack) speed pct)
    &Unit::HandleNULLProc,                                      //194 SPELL_AURA_MOD_IGNORE_ABSORB_SCHOOL
    &Unit::HandleNULLProc,                                      //195 SPELL_AURA_MOD_IGNORE_ABSORB_FOR_SPELL
    &Unit::HandleNULLProc,                                      //196 SPELL_AURA_MOD_COOLDOWN (single spell 24818 in 3.2.2a)
    &Unit::HandleNULLProc,                                      //197 SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCEe
    &Unit::HandleNULLProc,                                      //198 unused (3.0.8a-3.2.2a) old SPELL_AURA_MOD_ALL_WEAPON_SKILLS
    &Unit::HandleNULLProc,                                      //199 SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT
    &Unit::HandleNULLProc,                                      //200 SPELL_AURA_MOD_KILL_XP_PCT
    &Unit::HandleNULLProc,                                      //201 SPELL_AURA_FLY                             this aura enable flight mode...
    &Unit::HandleNULLProc,                                      //202 SPELL_AURA_CANNOT_BE_DODGED
    &Unit::HandleNULLProc,                                      //203 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE
    &Unit::HandleNULLProc,                                      //204 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE
    &Unit::HandleNULLProc,                                      //205 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_DAMAGE
    &Unit::HandleNULLProc,                                      //206 SPELL_AURA_MOD_FLIGHT_SPEED
    &Unit::HandleNULLProc,                                      //207 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED
    &Unit::HandleNULLProc,                                      //208 SPELL_AURA_MOD_FLIGHT_SPEED_STACKING
    &Unit::HandleNULLProc,                                      //209 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_STACKING
    &Unit::HandleNULLProc,                                      //210 SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING
    &Unit::HandleNULLProc,                                      //211 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_NOT_STACKING
    &Unit::HandleNULLProc,                                      //212 SPELL_AURA_MOD_RANGED_ATTACK_POWER_OF_STAT_PERCENT
    &Unit::HandleNULLProc,                                      //213 SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT implemented in Player::RewardRage
    &Unit::HandleNULLProc,                                      //214 Tamed Pet Passive (single test like spell 20782, also single for 157 aura)
    &Unit::HandleNULLProc,                                      //215 SPELL_AURA_ARENA_PREPARATION
    &Unit::HandleNULLProc,                                      //216 SPELL_AURA_HASTE_SPELLS
    &Unit::HandleNULLProc,                                      //217 unused (3.0.8a-3.2.2a)
    &Unit::HandleNULLProc,                                      //218 SPELL_AURA_HASTE_RANGED
    &Unit::HandleNULLProc,                                      //219 SPELL_AURA_MOD_MANA_REGEN_FROM_STAT
    &Unit::HandleNULLProc,                                      //220 SPELL_AURA_MOD_RATING_FROM_STAT
    &Unit::HandleNULLProc,                                      //221 ignored
    &Unit::HandleNULLProc,                                      //222 unused (3.0.8a-3.2.2a) only for spell 44586 that not used in real spell cast
    &Unit::HandleNULLProc,                                      //223 dummy code (cast damage spell to attacker) and another dymmy (jump to another nearby raid member)
    &Unit::HandleNULLProc,                                      //224 unused (3.0.8a-3.2.2a)
    &Unit::HandleMendingAuraProc,                               //225 SPELL_AURA_PRAYER_OF_MENDING
    &Unit::HandleNULLProc,                                      //226 SPELL_AURA_PERIODIC_DUMMY
    &Unit::HandleNULLProc,                                      //227 SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE
    &Unit::HandleNULLProc,                                      //228 SPELL_AURA_DETECT_STEALTH
    &Unit::HandleNULLProc,                                      //229 SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE
    &Unit::HandleNULLProc,                                      //230 Commanding Shout
    &Unit::HandleProcTriggerSpellAuraProc,                      //231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
    &Unit::HandleNULLProc,                                      //232 SPELL_AURA_MECHANIC_DURATION_MOD
    &Unit::HandleNULLProc,                                      //233 set model id to the one of the creature with id m_modifier.m_miscvalue
    &Unit::HandleNULLProc,                                      //234 SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK
    &Unit::HandleNULLProc,                                      //235 SPELL_AURA_MOD_DISPEL_RESIST
    &Unit::HandleNULLProc,                                      //236 SPELL_AURA_CONTROL_VEHICLE
    &Unit::HandleNULLProc,                                      //237 SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER
    &Unit::HandleNULLProc,                                      //238 SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER
    &Unit::HandleNULLProc,                                      //239 SPELL_AURA_MOD_SCALE_2 only in Noggenfogger Elixir (16595) before 2.3.0 aura 61
    &Unit::HandleNULLProc,                                      //240 SPELL_AURA_MOD_EXPERTISE
    &Unit::HandleNULLProc,                                      //241 Forces the player to move forward
    &Unit::HandleNULLProc,                                      //242 SPELL_AURA_MOD_SPELL_DAMAGE_FROM_HEALING (only 2 test spels in 3.2.2a)
    &Unit::HandleNULLProc,                                      //243 faction reaction override spells
    &Unit::HandleNULLProc,                                      //244 SPELL_AURA_COMPREHEND_LANGUAGE
    &Unit::HandleNULLProc,                                      //245 SPELL_AURA_MOD_DURATION_OF_MAGIC_EFFECTS
    &Unit::HandleNULLProc,                                      //246 SPELL_AURA_MOD_DURATION_OF_EFFECTS_BY_DISPEL
    &Unit::HandleNULLProc,                                      //247 target to become a clone of the caster
    &Unit::HandleNULLProc,                                      //248 SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
    &Unit::HandleNULLProc,                                      //249 SPELL_AURA_CONVERT_RUNE
    &Unit::HandleNULLProc,                                      //250 SPELL_AURA_MOD_INCREASE_HEALTH_2
    &Unit::HandleNULLProc,                                      //251 SPELL_AURA_MOD_ENEMY_DODGE
    &Unit::HandleNULLProc,                                      //252 SPELL_AURA_SLOW_ALL
    &Unit::HandleNULLProc,                                      //253 SPELL_AURA_MOD_BLOCK_CRIT_CHANCE
    &Unit::HandleNULLProc,                                      //254 SPELL_AURA_MOD_DISARM_SHIELD disarm Shield
    &Unit::HandleNULLProc,                                      //255 SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT
    &Unit::HandleNULLProc,                                      //256 SPELL_AURA_NO_REAGENT_USE Use SpellClassMask for spell select
    &Unit::HandleNULLProc,                                      //257 SPELL_AURA_MOD_TARGET_RESIST_BY_SPELL_CLASS Use SpellClassMask for spell select
    &Unit::HandleNULLProc,                                      //258 SPELL_AURA_MOD_SPELL_VISUAL
    &Unit::HandleNULLProc,                                      //259 corrupt healing over time spell
    &Unit::HandleNULLProc,                                      //260 SPELL_AURA_SCREEN_EFFECT (miscvalue = id in ScreenEffect.dbc) not required any code
    &Unit::HandleNULLProc,                                      //261 SPELL_AURA_PHASE undetectable invisibility?
    &Unit::HandleNULLProc,                                      //262 ignore combat/aura state?
    &Unit::HandleNULLProc,                                      //263 SPELL_AURA_ALLOW_ONLY_ABILITY player can use only abilities set in SpellClassMask
    &Unit::HandleNULLProc,                                      //264 unused (3.0.8a-3.2.2a)
    &Unit::HandleNULLProc,                                      //265 unused (3.0.8a-3.2.2a)
    &Unit::HandleNULLProc,                                      //266 unused (3.0.8a-3.2.2a)
    &Unit::HandleNULLProc,                                      //267 SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL
    &Unit::HandleNULLProc,                                      //268 SPELL_AURA_MOD_ATTACK_POWER_OF_STAT_PERCENT
    &Unit::HandleNULLProc,                                      //269 SPELL_AURA_MOD_IGNORE_DAMAGE_REDUCTION_SCHOOL
    &Unit::HandleNULLProc,                                      //270 SPELL_AURA_MOD_IGNORE_TARGET_RESIST (unused in 3.2.2a)
    &Unit::HandleModDamageFromCasterAuraProc,                   //271 SPELL_AURA_MOD_DAMAGE_FROM_CASTER
    &Unit::HandleMaelstromWeaponAuraProc,                       //272 SPELL_AURA_MAELSTROM_WEAPON (unclear use for aura, it used in (3.2.2a...3.3.0) in single spell 53817 that spellmode stacked and charged spell expected to be drop as stack
    &Unit::HandleNULLProc,                                      //273 SPELL_AURA_X_RAY (client side implementation)
    &Unit::HandleNULLProc,                                      //274 proc free shot?
    &Unit::HandleNULLProc,                                      //275 SPELL_AURA_MOD_IGNORE_SHAPESHIFT Use SpellClassMask for spell select
    &Unit::HandleNULLProc,                                      //276 mod damage % mechanic?
    &Unit::HandleNULLProc,                                      //277 SPELL_AURA_MOD_MAX_AFFECTED_TARGETS Use SpellClassMask for spell select
    &Unit::HandleNULLProc,                                      //278 SPELL_AURA_MOD_DISARM_RANGED disarm ranged weapon
    &Unit::HandleNULLProc,                                      //279 visual effects? 58836 and 57507
    &Unit::HandleNULLProc,                                      //280 SPELL_AURA_MOD_TARGET_ARMOR_PCT
    &Unit::HandleNULLProc,                                      //281 SPELL_AURA_MOD_HONOR_GAIN
    &Unit::HandleNULLProc,                                      //282 SPELL_AURA_INCREASE_BASE_HEALTH_PERCENT
    &Unit::HandleNULLProc,                                      //283 SPELL_AURA_MOD_HEALING_RECEIVED
    &Unit::HandleNULLProc,                                      //284 51 spells
    &Unit::HandleNULLProc,                                      //285 SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR
    &Unit::HandleNULLProc,                                      //286 SPELL_AURA_ABILITY_PERIODIC_CRIT
    &Unit::HandleNULLProc,                                      //287 SPELL_AURA_DEFLECT_SPELLS
    &Unit::HandleNULLProc,                                      //288 increase parry/deflect, prevent attack (single spell used 67801)
    &Unit::HandleNULLProc,                                      //289 unused (3.2.2a)
    &Unit::HandleNULLProc,                                      //290 SPELL_AURA_MOD_ALL_CRIT_CHANCE
    &Unit::HandleNULLProc,                                      //291 SPELL_AURA_MOD_QUEST_XP_PCT
    &Unit::HandleNULLProc,                                      //292 call stabled pet
    &Unit::HandleNULLProc,                                      //293 3 spells
    &Unit::HandleNULLProc,                                      //294 2 spells, possible prevent mana regen
    &Unit::HandleNULLProc,                                      //295 unused (3.2.2a)
    &Unit::HandleNULLProc,                                      //296 2 spells
    &Unit::HandleNULLProc,                                      //297 1 spell (counter spell school?)
    &Unit::HandleNULLProc,                                      //298 unused (3.2.2a)
    &Unit::HandleNULLProc,                                      //299 unused (3.2.2a)
    &Unit::HandleNULLProc,                                      //300 3 spells (share damage?)
    &Unit::HandleNULLProc,                                      //301 5 spells
    &Unit::HandleNULLProc,                                      //302 unused (3.2.2a)
    &Unit::HandleNULLProc,                                      //303 17 spells
    &Unit::HandleNULLProc,                                      //304 2 spells (alcohol effect?)
    &Unit::HandleNULLProc,                                      //305 SPELL_AURA_MOD_MINIMUM_SPEED
    &Unit::HandleNULLProc,                                      //306 1 spell
    &Unit::HandleNULLProc,                                      //307 absorb healing?
    &Unit::HandleNULLProc,                                      //308 new aura for hunter traps
    &Unit::HandleNULLProc,                                      //309 absorb healing?
    &Unit::HandleNULLProc,                                      //310 pet avoidance passive?
    &Unit::HandleNULLProc,                                      //311 0 spells in 3.3
    &Unit::HandleNULLProc,                                      //312 0 spells in 3.3
    &Unit::HandleNULLProc,                                      //313 0 spells in 3.3
    &Unit::HandleNULLProc,                                      //314 1 test spell (reduce duration of silince/magic)
    &Unit::HandleNULLProc,                                      //315 underwater walking
    &Unit::HandleNULLProc                                       //316 makes haste affect HOT/DOT ticks
};

bool Unit::IsTriggeredAtSpellProcEvent(Unit *pVictim, SpellAuraHolder* holder, SpellEntry const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, bool isVictim, SpellProcEventEntry const*& spellProcEvent )
{
    SpellEntry const* spellProto = holder->GetSpellProto ();

    // Get proc Event Entry
    spellProcEvent = sSpellMgr.GetSpellProcEvent(spellProto->Id);

    // Get EventProcFlag
    uint32 EventProcFlag;
    if (spellProcEvent && spellProcEvent->procFlags) // if exist get custom spellProcEvent->procFlags
        EventProcFlag = spellProcEvent->procFlags;
    else
        EventProcFlag = spellProto->procFlags;       // else get from spell proto
    // Continue if no trigger exist
    if (!EventProcFlag)
        return false;

    // Check spellProcEvent data requirements
    if(!SpellMgr::IsSpellProcEventCanTriggeredBy(spellProcEvent, EventProcFlag, procSpell, procFlag, procExtra))
        return false;

    // In most cases req get honor or XP from kill
    if (EventProcFlag & PROC_FLAG_KILL && GetTypeId() == TYPEID_PLAYER)
    {
        bool allow = ((Player*)this)->isHonorOrXPTarget(pVictim);
        // Shadow Word: Death - can trigger from every kill
        if (holder->GetId() == 32409)
            allow = true;
        if (!allow)
            return false;
    }
    // Aura added by spell can`t trogger from self (prevent drop charges/do triggers)
    // But except periodic triggers (can triggered from self)
    if(procSpell && procSpell->Id == spellProto->Id && !(spellProto->procFlags & PROC_FLAG_ON_TAKE_PERIODIC))
        return false;

    // Check if current equipment allows aura to proc
    if(!isVictim && GetTypeId() == TYPEID_PLAYER)
    {
        if(spellProto->EquippedItemClass == ITEM_CLASS_WEAPON)
        {
            Item *item = NULL;
            if(attType == BASE_ATTACK)
                item = ((Player*)this)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            else if (attType == OFF_ATTACK)
                item = ((Player*)this)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            else
                item = ((Player*)this)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);

            if(!item || item->IsBroken() || item->GetProto()->Class != ITEM_CLASS_WEAPON || !((1<<item->GetProto()->SubClass) & spellProto->EquippedItemSubClassMask))
                return false;
        }
        else if(spellProto->EquippedItemClass == ITEM_CLASS_ARMOR)
        {
            // Check if player is wearing shield
            Item *item = ((Player*)this)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if(!item || item->IsBroken() || item->GetProto()->Class != ITEM_CLASS_ARMOR || !((1<<item->GetProto()->SubClass) & spellProto->EquippedItemSubClassMask))
                return false;
        }
    }
    // Get chance from spell
    float chance = (float)spellProto->procChance;
    // If in spellProcEvent exist custom chance, chance = spellProcEvent->customChance;
    if(spellProcEvent && spellProcEvent->customChance)
        chance = spellProcEvent->customChance;
    // If PPM exist calculate chance from PPM
    if(!isVictim && spellProcEvent && spellProcEvent->ppmRate != 0)
    {
        uint32 WeaponSpeed = GetAttackTime(attType);
        chance = GetPPMProcChance(WeaponSpeed, spellProcEvent->ppmRate);
    }
    // Apply chance modifer aura
    if(Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_CHANCE_OF_SUCCESS,chance);
        modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_FREQUENCY_OF_SUCCESS,chance);
    }

    return roll_chance_f(chance);
}

SpellAuraProcResult Unit::HandleHasteAuraProc(Unit *pVictim, uint32 damage, Aura* triggeredByAura, SpellEntry const * /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 cooldown)
{
    SpellEntry const *hasteSpell = triggeredByAura->GetSpellProto();

    Item* castItem = triggeredByAura->GetCastItemGUID() && GetTypeId()==TYPEID_PLAYER
        ? ((Player*)this)->GetItemByGuid(triggeredByAura->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = pVictim;
    int32 basepoints0 = 0;

    switch(hasteSpell->SpellFamilyName)
    {
        case SPELLFAMILY_ROGUE:
        {
            switch(hasteSpell->Id)
            {
                // Blade Flurry
                case 13877:
                case 33735:
                {
                    target = SelectRandomUnfriendlyTarget(pVictim);
                    if(!target)
                        return SPELL_AURA_PROC_FAILED;
                    basepoints0 = damage;
                    triggered_spell_id = 22482;
                    break;
                }
            }
            break;
        }
    }

    // processed charge only counting case
    if(!triggered_spell_id)
        return SPELL_AURA_PROC_OK;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if(!triggerEntry)
    {
        sLog.outError("Unit::HandleHasteAuraProc: Spell %u have nonexistent triggered spell %u",hasteSpell->Id,triggered_spell_id);
        return SPELL_AURA_PROC_FAILED;
    }

    // default case
    if(!target || target!=this && !target->isAlive())
        return SPELL_AURA_PROC_FAILED;

    if( cooldown && GetTypeId()==TYPEID_PLAYER && ((Player*)this)->HasSpellCooldown(triggered_spell_id))
        return SPELL_AURA_PROC_FAILED;

    if(basepoints0)
        CastCustomSpell(target,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
    else
        CastSpell(target,triggered_spell_id,true,castItem,triggeredByAura);

    if( cooldown && GetTypeId()==TYPEID_PLAYER )
        ((Player*)this)->AddSpellCooldown(triggered_spell_id,0,time(NULL) + cooldown);

    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleSpellCritChanceAuraProc(Unit *pVictim, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const * procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 cooldown)
{
    if (!procSpell)
        return SPELL_AURA_PROC_FAILED;

    SpellEntry const *triggeredByAuraSpell = triggeredByAura->GetSpellProto();

    Item* castItem = triggeredByAura->GetCastItemGUID() && GetTypeId()==TYPEID_PLAYER
        ? ((Player*)this)->GetItemByGuid(triggeredByAura->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = pVictim;
    int32 basepoints0 = 0;

    switch(triggeredByAuraSpell->SpellFamilyName)
    {
        case SPELLFAMILY_MAGE:
        {
            switch(triggeredByAuraSpell->Id)
            {
                // Focus Magic
                case 54646:
                {
                    Unit* caster = triggeredByAura->GetCaster();
                    if(!caster)
                        return SPELL_AURA_PROC_FAILED;

                    triggered_spell_id = 54648;
                    target = caster;
                    break;
                }
            }
        }
    }

    // processed charge only counting case
    if(!triggered_spell_id)
        return SPELL_AURA_PROC_OK;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if(!triggerEntry)
    {
        sLog.outError("Unit::HandleHasteAuraProc: Spell %u have nonexistent triggered spell %u",triggeredByAuraSpell->Id,triggered_spell_id);
        return SPELL_AURA_PROC_FAILED;
    }

    // default case
    if(!target || target!=this && !target->isAlive())
        return SPELL_AURA_PROC_FAILED;

    if( cooldown && GetTypeId()==TYPEID_PLAYER && ((Player*)this)->HasSpellCooldown(triggered_spell_id))
        return SPELL_AURA_PROC_FAILED;

    if(basepoints0)
        CastCustomSpell(target,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
    else
        CastSpell(target,triggered_spell_id,true,castItem,triggeredByAura);

    if( cooldown && GetTypeId()==TYPEID_PLAYER )
        ((Player*)this)->AddSpellCooldown(triggered_spell_id,0,time(NULL) + cooldown);

    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleDummyAuraProc(Unit *pVictim, uint32 damage, Aura* triggeredByAura, SpellEntry const * procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown)
{
    SpellEntry const *dummySpell = triggeredByAura->GetSpellProto ();
    SpellEffectIndex effIndex = triggeredByAura->GetEffIndex();
    int32  triggerAmount = triggeredByAura->GetModifier()->m_amount;

    Item* castItem = triggeredByAura->GetCastItemGUID() && GetTypeId()==TYPEID_PLAYER
        ? ((Player*)this)->GetItemByGuid(triggeredByAura->GetCastItemGUID()) : NULL;

    // some dummy spells have trigger spell in spell data already (from 3.0.3)
    uint32 triggered_spell_id = dummySpell->EffectApplyAuraName[effIndex] == SPELL_AURA_DUMMY ? dummySpell->EffectTriggerSpell[effIndex] : 0;
    Unit* target = pVictim;
    int32  basepoints[MAX_EFFECT_INDEX] = {0, 0, 0};

    switch(dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (dummySpell->Id)
            {
                // Eye for an Eye
                case 9799:
                case 25988:
                {
                    // return damage % to attacker but < 50% own total health
                    basepoints[0] = triggerAmount*int32(damage)/100;
                    if (basepoints[0] > (int32)GetMaxHealth()/2)
                        basepoints[0] = (int32)GetMaxHealth()/2;

                    triggered_spell_id = 25997;
                    break;
                }
                // Sweeping Strikes (NPC spells may be)
                case 18765:
                case 35429:
                {
                    // prevent chain of triggered spell from same triggered spell
                    if (procSpell && procSpell->Id == 26654)
                        return SPELL_AURA_PROC_FAILED;

                    target = SelectRandomUnfriendlyTarget(pVictim);
                    if(!target)
                        return SPELL_AURA_PROC_FAILED;

                    triggered_spell_id = 26654;
                    break;
                }
                // Twisted Reflection (boss spell)
                case 21063:
                    triggered_spell_id = 21064;
                    break;
                // Unstable Power
                case 24658:
                {
                    if (!procSpell || procSpell->Id == 24659)
                        return SPELL_AURA_PROC_FAILED;
                    // Need remove one 24659 aura
                    RemoveAuraHolderFromStack(24659);
                    return SPELL_AURA_PROC_OK;
                }
                // Restless Strength
                case 24661:
                {
                    // Need remove one 24662 aura
                    RemoveAuraHolderFromStack(24662);
                    return SPELL_AURA_PROC_OK;
                }
                // Adaptive Warding (Frostfire Regalia set)
                case 28764:
                {
                    if(!procSpell)
                        return SPELL_AURA_PROC_FAILED;

                    // find Mage Armor
                    bool found = false;
                    AuraList const& mRegenInterupt = GetAurasByType(SPELL_AURA_MOD_MANA_REGEN_INTERRUPT);
                    for(AuraList::const_iterator iter = mRegenInterupt.begin(); iter != mRegenInterupt.end(); ++iter)
                    {
                        if(SpellEntry const* iterSpellProto = (*iter)->GetSpellProto())
                        {
                            if(iterSpellProto->SpellFamilyName==SPELLFAMILY_MAGE && (iterSpellProto->SpellFamilyFlags & UI64LIT(0x10000000)))
                            {
                                found=true;
                                break;
                            }
                        }
                    }
                    if(!found)
                        return SPELL_AURA_PROC_FAILED;

                    switch(GetFirstSchoolInMask(GetSpellSchoolMask(procSpell)))
                    {
                        case SPELL_SCHOOL_NORMAL:
                        case SPELL_SCHOOL_HOLY:
                            return SPELL_AURA_PROC_FAILED;                   // ignored
                        case SPELL_SCHOOL_FIRE:   triggered_spell_id = 28765; break;
                        case SPELL_SCHOOL_NATURE: triggered_spell_id = 28768; break;
                        case SPELL_SCHOOL_FROST:  triggered_spell_id = 28766; break;
                        case SPELL_SCHOOL_SHADOW: triggered_spell_id = 28769; break;
                        case SPELL_SCHOOL_ARCANE: triggered_spell_id = 28770; break;
                        default:
                            return SPELL_AURA_PROC_FAILED;
                    }

                    target = this;
                    break;
                }
                // Obsidian Armor (Justice Bearer`s Pauldrons shoulder)
                case 27539:
                {
                    if(!procSpell)
                        return SPELL_AURA_PROC_FAILED;

                    switch(GetFirstSchoolInMask(GetSpellSchoolMask(procSpell)))
                    {
                        case SPELL_SCHOOL_NORMAL:
                            return SPELL_AURA_PROC_FAILED;                   // ignore
                        case SPELL_SCHOOL_HOLY:   triggered_spell_id = 27536; break;
                        case SPELL_SCHOOL_FIRE:   triggered_spell_id = 27533; break;
                        case SPELL_SCHOOL_NATURE: triggered_spell_id = 27538; break;
                        case SPELL_SCHOOL_FROST:  triggered_spell_id = 27534; break;
                        case SPELL_SCHOOL_SHADOW: triggered_spell_id = 27535; break;
                        case SPELL_SCHOOL_ARCANE: triggered_spell_id = 27540; break;
                        default:
                            return SPELL_AURA_PROC_FAILED;
                    }

                    target = this;
                    break;
                }
                // Mana Leech (Passive) (Priest Pet Aura)
                case 28305:
                {
                    // Cast on owner
                    target = GetOwner();
                    if(!target)
                        return SPELL_AURA_PROC_FAILED;

                    triggered_spell_id = 34650;
                    break;
                }
                // Divine purpose
                case 31871:
                case 31872:
                {
                    // Roll chance
                    if (!roll_chance_i(triggerAmount))
                        return SPELL_AURA_PROC_FAILED;

                    // Remove any stun effect on target
                    SpellAuraHolderMap& Auras = pVictim->GetSpellAuraHolderMap();
                    for(SpellAuraHolderMap::const_iterator iter = Auras.begin(); iter != Auras.end();)
                    {
                        SpellEntry const *spell = iter->second->GetSpellProto();

                        if( spell->Mechanic == MECHANIC_STUN ||
                            iter->second->HasMechanic(MECHANIC_STUN))
                        {
                            pVictim->RemoveAurasDueToSpell(spell->Id);
                            iter = Auras.begin();
                        }
                        else
                            ++iter;
                    }
                    return SPELL_AURA_PROC_OK;
                }
                // Mark of Malice
                case 33493:
                {
                    // Cast finish spell at last charge
                    if (triggeredByAura->GetHolder()->GetAuraCharges() > 1)
                        return SPELL_AURA_PROC_FAILED;

                    target = this;
                    triggered_spell_id = 33494;
                    break;
                }
                // Vampiric Aura (boss spell)
                case 38196:
                {
                    basepoints[0] = 3 * damage;               // 300%
                    if (basepoints[0] < 0)
                        return SPELL_AURA_PROC_FAILED;

                    triggered_spell_id = 31285;
                    target = this;
                    break;
                }
                // Aura of Madness (Darkmoon Card: Madness trinket)
                //=====================================================
                // 39511 Sociopath: +35 strength (Paladin, Rogue, Druid, Warrior)
                // 40997 Delusional: +70 attack power (Rogue, Hunter, Paladin, Warrior, Druid)
                // 40998 Kleptomania: +35 agility (Warrior, Rogue, Paladin, Hunter, Druid)
                // 40999 Megalomania: +41 damage/healing (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                // 41002 Paranoia: +35 spell/melee/ranged crit strike rating (All classes)
                // 41005 Manic: +35 haste (spell, melee and ranged) (All classes)
                // 41009 Narcissism: +35 intellect (Druid, Shaman, Priest, Warlock, Mage, Paladin, Hunter)
                // 41011 Martyr Complex: +35 stamina (All classes)
                // 41406 Dementia: Every 5 seconds either gives you +5% damage/healing. (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                // 41409 Dementia: Every 5 seconds either gives you -5% damage/healing. (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                case 39446:
                {
                    if(GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    // Select class defined buff
                    switch (getClass())
                    {
                        case CLASS_PALADIN:                 // 39511,40997,40998,40999,41002,41005,41009,41011,41409
                        case CLASS_DRUID:                   // 39511,40997,40998,40999,41002,41005,41009,41011,41409
                        {
                            uint32 RandomSpell[]={39511,40997,40998,40999,41002,41005,41009,41011,41409};
                            triggered_spell_id = RandomSpell[ irand(0, sizeof(RandomSpell)/sizeof(uint32) - 1) ];
                            break;
                        }
                        case CLASS_ROGUE:                   // 39511,40997,40998,41002,41005,41011
                        case CLASS_WARRIOR:                 // 39511,40997,40998,41002,41005,41011
                        {
                            uint32 RandomSpell[]={39511,40997,40998,41002,41005,41011};
                            triggered_spell_id = RandomSpell[ irand(0, sizeof(RandomSpell)/sizeof(uint32) - 1) ];
                            break;
                        }
                        case CLASS_PRIEST:                  // 40999,41002,41005,41009,41011,41406,41409
                        case CLASS_SHAMAN:                  // 40999,41002,41005,41009,41011,41406,41409
                        case CLASS_MAGE:                    // 40999,41002,41005,41009,41011,41406,41409
                        case CLASS_WARLOCK:                 // 40999,41002,41005,41009,41011,41406,41409
                        {
                            uint32 RandomSpell[]={40999,41002,41005,41009,41011,41406,41409};
                            triggered_spell_id = RandomSpell[ irand(0, sizeof(RandomSpell)/sizeof(uint32) - 1) ];
                            break;
                        }
                        case CLASS_HUNTER:                  // 40997,40999,41002,41005,41009,41011,41406,41409
                        {
                            uint32 RandomSpell[]={40997,40999,41002,41005,41009,41011,41406,41409};
                            triggered_spell_id = RandomSpell[ irand(0, sizeof(RandomSpell)/sizeof(uint32) - 1) ];
                            break;
                        }
                        default:
                            return SPELL_AURA_PROC_FAILED;
                    }

                    target = this;
                    if (roll_chance_i(10))
                        ((Player*)this)->Say("This is Madness!", LANG_UNIVERSAL);
                    break;
                }
                // Sunwell Exalted Caster Neck (Shattered Sun Pendant of Acumen neck)
                // cast 45479 Light's Wrath if Exalted by Aldor
                // cast 45429 Arcane Bolt if Exalted by Scryers
                case 45481:
                {
                    if(GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    // Get Aldor reputation rank
                    if (((Player *)this)->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45479;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (((Player *)this)->GetReputationRank(934) == REP_EXALTED)
                    {
                        // triggered at positive/self casts also, current attack target used then
                        if(IsFriendlyTo(target))
                        {
                            target = getVictim();
                            if(!target)
                            {
                                uint64 selected_guid = ((Player *)this)->GetSelection();
                                target = ObjectAccessor::GetUnit(*this,selected_guid);
                                if(!target)
                                    return SPELL_AURA_PROC_FAILED;
                            }
                            if(IsFriendlyTo(target))
                                return SPELL_AURA_PROC_FAILED;
                        }

                        triggered_spell_id = 45429;
                        break;
                    }
                    return SPELL_AURA_PROC_FAILED;
                }
                // Sunwell Exalted Melee Neck (Shattered Sun Pendant of Might neck)
                // cast 45480 Light's Strength if Exalted by Aldor
                // cast 45428 Arcane Strike if Exalted by Scryers
                case 45482:
                {
                    if(GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    // Get Aldor reputation rank
                    if (((Player *)this)->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45480;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (((Player *)this)->GetReputationRank(934) == REP_EXALTED)
                    {
                        triggered_spell_id = 45428;
                        break;
                    }
                    return SPELL_AURA_PROC_FAILED;
                }
                // Sunwell Exalted Tank Neck (Shattered Sun Pendant of Resolve neck)
                // cast 45431 Arcane Insight if Exalted by Aldor
                // cast 45432 Light's Ward if Exalted by Scryers
                case 45483:
                {
                    if(GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    // Get Aldor reputation rank
                    if (((Player *)this)->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45432;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (((Player *)this)->GetReputationRank(934) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45431;
                        break;
                    }
                    return SPELL_AURA_PROC_FAILED;
                }
                // Sunwell Exalted Healer Neck (Shattered Sun Pendant of Restoration neck)
                // cast 45478 Light's Salvation if Exalted by Aldor
                // cast 45430 Arcane Surge if Exalted by Scryers
                case 45484:
                {
                    if(GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    // Get Aldor reputation rank
                    if (((Player *)this)->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45478;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (((Player *)this)->GetReputationRank(934) == REP_EXALTED)
                    {
                        triggered_spell_id = 45430;
                        break;
                    }
                    return SPELL_AURA_PROC_FAILED;
                }
                /*
                // Sunwell Exalted Caster Neck (??? neck)
                // cast ??? Light's Wrath if Exalted by Aldor
                // cast ??? Arcane Bolt if Exalted by Scryers*/
                case 46569:
                    return SPELL_AURA_PROC_FAILED;                           // old unused version
                // Living Seed
                case 48504:
                {
                    triggered_spell_id = 48503;
                    basepoints[0] = triggerAmount;
                    target = this;
                    break;
                }
                // Vampiric Touch (generic, used by some boss)
                case 52723:
                case 60501:
                {
                    triggered_spell_id = 52724;
                    basepoints[0] = damage / 2;
                    target = this;
                    break;
                }
                // Shadowfiend Death (Gain mana if pet dies with Glyph of Shadowfiend)
                case 57989:
                {
                    Unit *owner = GetOwner();
                    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    // Glyph of Shadowfiend (need cast as self cast for owner, no hidden cooldown)
                    owner->CastSpell(owner,58227,true,castItem,triggeredByAura);
                    return SPELL_AURA_PROC_OK;
                }
                // Glyph of Life Tap
                case 63320:
                    triggered_spell_id = 63321;
                    break;
                // Item - Shadowmourne Legendary
                case 71903:
                {
                    if (!roll_chance_i(triggerAmount))
                        return SPELL_AURA_PROC_FAILED;

                    SpellAuraHolder *aurHolder = GetSpellAuraHolder(71905);
                    if (aurHolder && uint32(aurHolder->GetStackAmount() + 1) >= aurHolder->GetSpellProto()->StackAmount)
                    {
                        RemoveAurasDueToSpell(71905);
                        CastSpell(this, 71904, true);       // Chaos Bane
                        return SPELL_AURA_PROC_OK;
                    }
                    else
                        triggered_spell_id = 71905;

                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // Magic Absorption
            if (dummySpell->SpellIconID == 459)             // only this spell have SpellIconID == 459 and dummy aura
            {
                if (getPowerType() != POWER_MANA)
                    return SPELL_AURA_PROC_FAILED;

                // mana reward
                basepoints[0] = (triggerAmount * GetMaxPower(POWER_MANA) / 100);
                target = this;
                triggered_spell_id = 29442;
                break;
            }
            // Master of Elements
            if (dummySpell->SpellIconID == 1920)
            {
                if(!procSpell)
                    return SPELL_AURA_PROC_FAILED;

                // mana cost save
                int32 cost = procSpell->manaCost + procSpell->ManaCostPercentage * GetCreateMana() / 100;
                basepoints[0] = cost * triggerAmount/100;
                if (basepoints[0] <=0)
                    return SPELL_AURA_PROC_FAILED;

                target = this;
                triggered_spell_id = 29077;
                break;
            }

            // Arcane Potency
            if (dummySpell->SpellIconID == 2120)
            {
                if(!procSpell)
                    return SPELL_AURA_PROC_FAILED;

                target = this;
                switch (dummySpell->Id)
                {
                    case 31571: triggered_spell_id = 57529; break;
                    case 31572: triggered_spell_id = 57531; break;
                    default:
                        sLog.outError("Unit::HandleDummyAuraProc: non handled spell id: %u",dummySpell->Id);
                        return SPELL_AURA_PROC_FAILED;
                }
                break;
            }

            // Hot Streak
            if (dummySpell->SpellIconID == 2999)
            {
                if (effIndex != EFFECT_INDEX_0)
                    return SPELL_AURA_PROC_OK;
                Aura *counter = GetAura(triggeredByAura->GetId(), EFFECT_INDEX_1);
                if (!counter)
                    return SPELL_AURA_PROC_OK;

                // Count spell criticals in a row in second aura
                Modifier *mod = counter->GetModifier();
                if (procEx & PROC_EX_CRITICAL_HIT)
                {
                    mod->m_amount *=2;
                    if (mod->m_amount < 100) // not enough
                        return SPELL_AURA_PROC_OK;
                    // Crititcal counted -> roll chance
                    if (roll_chance_i(triggerAmount))
                        CastSpell(this, 48108, true, castItem, triggeredByAura);
                }
                mod->m_amount = 25;
                return SPELL_AURA_PROC_OK;
            }
            // Burnout
            if (dummySpell->SpellIconID == 2998)
            {
                if(!procSpell)
                    return SPELL_AURA_PROC_FAILED;

                int32 cost = procSpell->manaCost + procSpell->ManaCostPercentage * GetCreateMana() / 100;
                basepoints[0] = cost * triggerAmount/100;
                if (basepoints[0] <=0)
                    return SPELL_AURA_PROC_FAILED;
                triggered_spell_id = 44450;
                target = this;
                break;
            }
            // Incanter's Regalia set (add trigger chance to Mana Shield)
            if (dummySpell->SpellFamilyFlags & UI64LIT(0x0000000000008000))
            {
                if (GetTypeId() != TYPEID_PLAYER)
                    return SPELL_AURA_PROC_FAILED;

                target = this;
                triggered_spell_id = 37436;
                break;
            }
            switch(dummySpell->Id)
            {
                // Ignite
                case 11119:
                case 11120:
                case 12846:
                case 12847:
                case 12848:
                {
                    switch (dummySpell->Id)
                    {
                        case 11119: basepoints[0] = int32(0.04f*damage); break;
                        case 11120: basepoints[0] = int32(0.08f*damage); break;
                        case 12846: basepoints[0] = int32(0.12f*damage); break;
                        case 12847: basepoints[0] = int32(0.16f*damage); break;
                        case 12848: basepoints[0] = int32(0.20f*damage); break;
                        default:
                            sLog.outError("Unit::HandleDummyAuraProc: non handled spell id: %u (IG)",dummySpell->Id);
                            return SPELL_AURA_PROC_FAILED;
                    }

                    triggered_spell_id = 12654;
                    break;
                }
                // Glyph of Ice Block
                case 56372:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    // not 100% safe with client version switches but for 3.1.3 no spells with cooldown that can have mage player except Frost Nova.
                    ((Player*)this)->RemoveSpellCategoryCooldown(35, true);
                    return SPELL_AURA_PROC_OK;
                }
                // Glyph of Polymorph
                case 56375:
                {
                    if (!pVictim || !pVictim->isAlive())
                        return SPELL_AURA_PROC_FAILED;

                    pVictim->RemoveSpellsCausingAura(SPELL_AURA_PERIODIC_DAMAGE);
                    pVictim->RemoveSpellsCausingAura(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                    return SPELL_AURA_PROC_OK;
                }
                // Blessing of Ancient Kings
                case 64411:
                {
                    // for DOT procs
                    if (!IsPositiveSpell(procSpell->Id))
                        return SPELL_AURA_PROC_FAILED;

                    triggered_spell_id = 64413;
                    basepoints[0] = damage * 15 / 100;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Retaliation
            if (dummySpell->SpellFamilyFlags == UI64LIT(0x0000000800000000))
            {
                // check attack comes not from behind
                if (!HasInArc(M_PI_F, pVictim))
                    return SPELL_AURA_PROC_FAILED;

                triggered_spell_id = 22858;
                break;
            }
            // Second Wind
            if (dummySpell->SpellIconID == 1697)
            {
                // only for spells and hit/crit (trigger start always) and not start from self casted spells (5530 Mace Stun Effect for example)
                if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == pVictim)
                    return SPELL_AURA_PROC_FAILED;
                // Need stun or root mechanic
                if (!(GetAllSpellMechanicMask(procSpell) & IMMUNE_TO_ROOT_AND_STUN_MASK))
                    return SPELL_AURA_PROC_FAILED;

                switch (dummySpell->Id)
                {
                    case 29838: triggered_spell_id=29842; break;
                    case 29834: triggered_spell_id=29841; break;
                    case 42770: triggered_spell_id=42771; break;
                    default:
                        sLog.outError("Unit::HandleDummyAuraProc: non handled spell id: %u (SW)",dummySpell->Id);
                    return SPELL_AURA_PROC_FAILED;
                }

                target = this;
                break;
            }
            // Damage Shield
            if (dummySpell->SpellIconID == 3214)
            {
                triggered_spell_id = 59653;
                basepoints[0] = GetShieldBlockValue() * triggerAmount / 100;
                break;
            }

            // Sweeping Strikes
            if (dummySpell->Id == 12328)
            {
                // prevent chain of triggered spell from same triggered spell
                if(procSpell && procSpell->Id == 26654)
                    return SPELL_AURA_PROC_FAILED;

                target = SelectRandomUnfriendlyTarget(pVictim);
                if(!target)
                    return SPELL_AURA_PROC_FAILED;

                triggered_spell_id = 26654;
                break;
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Seed of Corruption
            if (dummySpell->SpellFamilyFlags & UI64LIT(0x0000001000000000))
            {
                Modifier* mod = triggeredByAura->GetModifier();
                // if damage is more than need or target die from damage deal finish spell
                if( mod->m_amount <= (int32)damage || GetHealth() <= damage )
                {
                    // remember guid before aura delete
                    uint64 casterGuid = triggeredByAura->GetCasterGUID();

                    // Remove aura (before cast for prevent infinite loop handlers)
                    RemoveAurasDueToSpell(triggeredByAura->GetId());

                    // Cast finish spell (triggeredByAura already not exist!)
                    CastSpell(this, 27285, true, castItem, NULL, casterGuid);
                    return SPELL_AURA_PROC_OK;                            // no hidden cooldown
                }

                // Damage counting
                mod->m_amount-=damage;
                return SPELL_AURA_PROC_OK;
            }
            // Seed of Corruption (Mobs cast) - no die req
            if (dummySpell->SpellFamilyFlags == UI64LIT(0x0) && dummySpell->SpellIconID == 1932)
            {
                Modifier* mod = triggeredByAura->GetModifier();
                // if damage is more than need deal finish spell
                if( mod->m_amount <= (int32)damage )
                {
                    // remember guid before aura delete
                    uint64 casterGuid = triggeredByAura->GetCasterGUID();

                    // Remove aura (before cast for prevent infinite loop handlers)
                    RemoveAurasDueToSpell(triggeredByAura->GetId());

                    // Cast finish spell (triggeredByAura already not exist!)
                    CastSpell(this, 32865, true, castItem, NULL, casterGuid);
                    return SPELL_AURA_PROC_OK;                            // no hidden cooldown
                }
                // Damage counting
                mod->m_amount-=damage;
                return SPELL_AURA_PROC_OK;
            }
            // Fel Synergy
            if (dummySpell->SpellIconID == 3222)
            {
                target = GetPet();
                if (!target)
                    return SPELL_AURA_PROC_FAILED;
                basepoints[0] = damage * triggerAmount / 100;
                triggered_spell_id = 54181;
                break;
            }
            switch(dummySpell->Id)
            {
                // Nightfall & Glyph of Corruption
                case 18094:
                case 18095:
                case 56218:
                {
                    target = this;
                    triggered_spell_id = 17941;
                    break;
                }
                //Soul Leech
                case 30293:
                case 30295:
                case 30296:
                {
                    // health
                    basepoints[0] = int32(damage*triggerAmount/100);
                    target = this;
                    triggered_spell_id = 30294;
                    break;
                }
                // Shadowflame (Voidheart Raiment set bonus)
                case 37377:
                {
                    triggered_spell_id = 37379;
                    break;
                }
                // Pet Healing (Corruptor Raiment or Rift Stalker Armor)
                case 37381:
                {
                    target = GetPet();
                    if (!target)
                        return SPELL_AURA_PROC_FAILED;

                    // heal amount
                    basepoints[0] = damage * triggerAmount/100;
                    triggered_spell_id = 37382;
                    break;
                }
                // Shadowflame Hellfire (Voidheart Raiment set bonus)
                case 39437:
                {
                    triggered_spell_id = 37378;
                    break;
                }
                // Siphon Life
                case 63108:
                {
                    // Glyph of Siphon Life
                    if (Aura *aur = GetAura(56216, EFFECT_INDEX_0))
                        triggerAmount += triggerAmount * aur->GetModifier()->m_amount / 100;

                    basepoints[0] = int32(damage * triggerAmount / 100);
                    triggered_spell_id = 63106;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Vampiric Touch
            if (dummySpell->SpellFamilyFlags & UI64LIT(0x0000040000000000))
            {
                if(!pVictim || !pVictim->isAlive())
                    return SPELL_AURA_PROC_FAILED;

                // pVictim is caster of aura
                if(triggeredByAura->GetCasterGUID() != pVictim->GetGUID())
                    return SPELL_AURA_PROC_FAILED;

                // Energize 0.25% of max. mana
                pVictim->CastSpell(pVictim,57669,true,castItem,triggeredByAura);
                return SPELL_AURA_PROC_OK;                                // no hidden cooldown
            }

            switch(dummySpell->SpellIconID)
            {
                // Improved Shadowform
                case 217:
                {
                    if(!roll_chance_i(triggerAmount))
                        return SPELL_AURA_PROC_FAILED;

                    RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
                    RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
                    break;
                }
                // Divine Aegis
                case 2820:
                {
                    basepoints[0] = damage * triggerAmount/100;
                    triggered_spell_id = 47753;
                    break;
                }
                // Empowered Renew
                case 3021:
                {
                    if (!procSpell)
                        return SPELL_AURA_PROC_FAILED;

                    // Renew
                    Aura* healingAura = pVictim->GetAura(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_PRIEST, UI64LIT(0x40), 0, GetGUID());
                    if (!healingAura)
                        return SPELL_AURA_PROC_FAILED;

                    int32 healingfromticks = healingAura->GetModifier()->m_amount * GetSpellAuraMaxTicks(procSpell);

                    basepoints[0] = healingfromticks * triggerAmount / 100;
                    triggered_spell_id = 63544;
                    break;
                }
                // Improved Devouring Plague
                case 3790:
                {
                    if (!procSpell)
                        return SPELL_AURA_PROC_FAILED;

                    Aura* leachAura = pVictim->GetAura(SPELL_AURA_PERIODIC_LEECH, SPELLFAMILY_PRIEST, UI64LIT(0x02000000), 0, GetGUID());
                    if (!leachAura)
                        return SPELL_AURA_PROC_FAILED;

                    int32 damagefromticks = leachAura->GetModifier()->m_amount * GetSpellAuraMaxTicks(procSpell);
                    basepoints[0] = damagefromticks * triggerAmount / 100;
                    triggered_spell_id = 63675;
                    break;
                }
            }

            switch(dummySpell->Id)
            {
                // Vampiric Embrace
                case 15286:
                {
                    // Return if self damage
                    if (this == pVictim)
                        return SPELL_AURA_PROC_FAILED;

                    // Heal amount - Self/Team
                    int32 team = triggerAmount*damage/500;
                    int32 self = triggerAmount*damage/100 - team;
                    CastCustomSpell(this,15290,&team,&self,NULL,true,castItem,triggeredByAura);
                    return SPELL_AURA_PROC_OK;                                // no hidden cooldown
                }
                // Priest Tier 6 Trinket (Ashtongue Talisman of Acumen)
                case 40438:
                {
                    // Shadow Word: Pain
                    if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000008000))
                        triggered_spell_id = 40441;
                    // Renew
                    else if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000010))
                        triggered_spell_id = 40440;
                    else
                        return SPELL_AURA_PROC_FAILED;

                    target = this;
                    break;
                }
                // Oracle Healing Bonus ("Garments of the Oracle" set)
                case 26169:
                {
                    // heal amount
                    basepoints[0] = int32(damage * 10/100);
                    target = this;
                    triggered_spell_id = 26170;
                    break;
                }
                // Frozen Shadoweave (Shadow's Embrace set) warning! its not only priest set
                case 39372:
                {
                    if(!procSpell || (GetSpellSchoolMask(procSpell) & (SPELL_SCHOOL_MASK_FROST | SPELL_SCHOOL_MASK_SHADOW))==0 )
                        return SPELL_AURA_PROC_FAILED;

                    // heal amount
                    basepoints[0] = damage * triggerAmount/100;
                    target = this;
                    triggered_spell_id = 39373;
                    break;
                }
                // Greater Heal (Vestments of Faith (Priest Tier 3) - 4 pieces bonus)
                case 28809:
                {
                    triggered_spell_id = 28810;
                    break;
                }
                // Glyph of Dispel Magic
                case 55677:
                {
                    if(!target->IsFriendlyTo(this))
                        return SPELL_AURA_PROC_FAILED;

                    basepoints[0] = int32(target->GetMaxHealth() * triggerAmount / 100);
                    // triggered_spell_id in spell data
                    break;
                }
                // Glyph of Prayer of Healing
                case 55680:
                {
                    basepoints[0] = int32(damage * triggerAmount  / 200);   // 10% each tick
                    triggered_spell_id = 56161;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch(dummySpell->Id)
            {
                // Leader of the Pack
                case 24932:
                {
                    // dummy m_amount store health percent (!=0 if Improved Leader of the Pack applied)
                    int32 heal_percent = triggeredByAura->GetModifier()->m_amount;
                    if (!heal_percent)
                        return SPELL_AURA_PROC_FAILED;

                    // check explicitly only to prevent mana cast when halth cast cooldown
                    if (cooldown && ((Player*)this)->HasSpellCooldown(34299))
                        return SPELL_AURA_PROC_FAILED;

                    // health
                    triggered_spell_id = 34299;
                    basepoints[0] = GetMaxHealth() * heal_percent / 100;
                    target = this;

                    // mana to caster
                    if (triggeredByAura->GetCasterGUID() == GetGUID())
                    {
                        if (SpellEntry const* manaCastEntry = sSpellStore.LookupEntry(60889))
                        {
                            int32 mana_percent = manaCastEntry->CalculateSimpleValue(EFFECT_INDEX_0) * heal_percent;
                            CastCustomSpell(this, manaCastEntry, &mana_percent, NULL, NULL, true, castItem, triggeredByAura);
                        }
                    }
                    break;
                }
                // Healing Touch (Dreamwalker Raiment set)
                case 28719:
                {
                    // mana back
                    basepoints[0] = int32(procSpell->manaCost * 30 / 100);
                    target = this;
                    triggered_spell_id = 28742;
                    break;
                }
                // Healing Touch Refund (Idol of Longevity trinket)
                case 28847:
                {
                    target = this;
                    triggered_spell_id = 28848;
                    break;
                }
                // Mana Restore (Malorne Raiment set / Malorne Regalia set)
                case 37288:
                case 37295:
                {
                    target = this;
                    triggered_spell_id = 37238;
                    break;
                }
                // Druid Tier 6 Trinket
                case 40442:
                {
                    float  chance;

                    // Starfire
                    if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000004))
                    {
                        triggered_spell_id = 40445;
                        chance = 25.0f;
                    }
                    // Rejuvenation
                    else if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000010))
                    {
                        triggered_spell_id = 40446;
                        chance = 25.0f;
                    }
                    // Mangle (Bear) and Mangle (Cat)
                    else if (procSpell->SpellFamilyFlags & UI64LIT(0x0000044000000000))
                    {
                        triggered_spell_id = 40452;
                        chance = 40.0f;
                    }
                    else
                        return SPELL_AURA_PROC_FAILED;

                    if (!roll_chance_f(chance))
                        return SPELL_AURA_PROC_FAILED;

                    target = this;
                    break;
                }
                // Maim Interrupt
                case 44835:
                {
                    // Deadly Interrupt Effect
                    triggered_spell_id = 32747;
                    break;
                }
                // Glyph of Rejuvenation
                case 54754:
                {
                    // less 50% health
                    if (pVictim->GetMaxHealth() < 2 * pVictim->GetHealth())
                        return SPELL_AURA_PROC_FAILED;
                    basepoints[0] = triggerAmount * damage / 100;
                    triggered_spell_id = 54755;
                    break;
                }
                // Item - Druid T10 Restoration 4P Bonus (Rejuvenation)
                case 70664:
                {
                    if (!procSpell || GetTypeId() != TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    float radius;
                    if (procSpell->EffectRadiusIndex[EFFECT_INDEX_0])
                        radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(procSpell->EffectRadiusIndex[EFFECT_INDEX_0]));
                    else
                        radius = GetSpellMaxRange(sSpellRangeStore.LookupEntry(procSpell->rangeIndex));

                    ((Player*)this)->ApplySpellMod(procSpell->Id, SPELLMOD_RADIUS, radius,NULL);

                    Unit *second = pVictim->SelectRandomFriendlyTarget(pVictim, radius);

                    if (!second)
                        return SPELL_AURA_PROC_FAILED;

                    pVictim->CastSpell(second, procSpell, true, NULL, triggeredByAura, GetGUID());
                    return SPELL_AURA_PROC_OK;
                }
            }
            // Eclipse
            if (dummySpell->SpellIconID == 2856)
            {
                if (!procSpell)
                    return SPELL_AURA_PROC_FAILED;

                // Wrath crit
                if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000001))
                {
                    if (HasAura(48517))
                        return SPELL_AURA_PROC_FAILED;
                    if (!roll_chance_i(60))
                        return SPELL_AURA_PROC_FAILED;
                    triggered_spell_id = 48518;
                    target = this;
                    break;
                }
                // Starfire crit
                if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000004))
                {
                    if (HasAura(48518))
                        return SPELL_AURA_PROC_FAILED;
                    triggered_spell_id = 48517;
                    target = this;
                    break;
                }
                return SPELL_AURA_PROC_FAILED;
            }
            // Living Seed
            else if (dummySpell->SpellIconID == 2860)
            {
                triggered_spell_id = 48504;
                basepoints[0] = triggerAmount * damage / 100;
                break;
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch(dummySpell->Id)
            {
                // Deadly Throw Interrupt
                case 32748:
                {
                    // Prevent cast Deadly Throw Interrupt on self from last effect (apply dummy) of Deadly Throw
                    if (this == pVictim)
                        return SPELL_AURA_PROC_FAILED;

                    triggered_spell_id = 32747;
                    break;
                }
            }
            // Cut to the Chase
            if (dummySpell->SpellIconID == 2909)
            {
                // "refresh your Slice and Dice duration to its 5 combo point maximum"
                // lookup Slice and Dice
                AuraList const& sd = GetAurasByType(SPELL_AURA_MOD_HASTE);
                for(AuraList::const_iterator itr = sd.begin(); itr != sd.end(); ++itr)
                {
                    SpellEntry const *spellProto = (*itr)->GetSpellProto();
                    if (spellProto->SpellFamilyName == SPELLFAMILY_ROGUE &&
                        (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000040000)))
                    {
                        (*itr)->GetHolder()->RefreshHolder();
                        return SPELL_AURA_PROC_OK;
                    }
                }
                return SPELL_AURA_PROC_FAILED;
            }
            // Deadly Brew
            if (dummySpell->SpellIconID == 2963)
            {
                triggered_spell_id = 44289;
                break;
            }
            // Quick Recovery
            if (dummySpell->SpellIconID == 2116)
            {
                if(!procSpell)
                    return SPELL_AURA_PROC_FAILED;

                // energy cost save
                basepoints[0] = procSpell->manaCost * triggerAmount/100;
                if (basepoints[0] <= 0)
                    return SPELL_AURA_PROC_FAILED;

                target = this;
                triggered_spell_id = 31663;
                break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Thrill of the Hunt
            if (dummySpell->SpellIconID == 2236)
            {
                if (!procSpell)
                    return SPELL_AURA_PROC_FAILED;

                // mana cost save
                int32 mana = procSpell->manaCost + procSpell->ManaCostPercentage * GetCreateMana() / 100;
                basepoints[0] = mana * 40/100;
                if (basepoints[0] <= 0)
                    return SPELL_AURA_PROC_FAILED;

                target = this;
                triggered_spell_id = 34720;
                break;
            }
            // Hunting Party
            if (dummySpell->SpellIconID == 3406)
            {
                triggered_spell_id = 57669;
                target = this;
                break;
            }
            // Lock and Load
            if ( dummySpell->SpellIconID == 3579 )
            {
                // Proc only from periodic (from trap activation proc another aura of this spell)
                if (!(procFlag & PROC_FLAG_ON_DO_PERIODIC) || !roll_chance_i(triggerAmount))
                    return SPELL_AURA_PROC_FAILED;
                triggered_spell_id = 56453;
                target = this;
                break;
            }
            // Rapid Recuperation
            if ( dummySpell->SpellIconID == 3560 )
            {
                // This effect only from Rapid Killing (mana regen)
                if (!(procSpell->SpellFamilyFlags & UI64LIT(0x0100000000000000)))
                    return SPELL_AURA_PROC_FAILED;

                target = this;

                switch(dummySpell->Id)
                {
                    case 53228:                             // Rank 1
                        triggered_spell_id = 56654;
                        break;
                    case 53232:                             // Rank 2
                        triggered_spell_id = 58882;
                        break;
                }
                break;
            }
            // Glyph of Mend Pet
            if(dummySpell->Id == 57870)
            {
                pVictim->CastSpell(pVictim, 57894, true, NULL, NULL, GetGUID());
                return SPELL_AURA_PROC_OK;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Seal of Righteousness - melee proc dummy (addition ${$MWS*(0.022*$AP+0.044*$SPH)} damage)
            if ((dummySpell->SpellFamilyFlags & UI64LIT(0x000000008000000)) && effIndex == EFFECT_INDEX_0)
            {
                triggered_spell_id = 25742;
                float ap = GetTotalAttackPowerValue(BASE_ATTACK);
                int32 holy = SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_HOLY);
                if (holy < 0)
                    holy = 0;
                basepoints[0] = GetAttackTime(BASE_ATTACK) * int32(ap*0.022f + 0.044f * holy) / 1000;
                break;
            }
            // Righteous Vengeance
            if (dummySpell->SpellIconID == 3025)
            {
                // 4 damage tick
                basepoints[0] = triggerAmount*damage/400;
                triggered_spell_id = 61840;
                break;
            }
            // Sheath of Light
            if (dummySpell->SpellIconID == 3030)
            {
                // 4 healing tick
                basepoints[0] = triggerAmount*damage/400;
                triggered_spell_id = 54203;
                break;
            }
            switch(dummySpell->Id)
            {
                // Judgement of Light
                case 20185:
                {
                    basepoints[0] = int32( pVictim->GetMaxHealth() * triggeredByAura->GetModifier()->m_amount / 100 );
                    pVictim->CastCustomSpell(pVictim, 20267, &basepoints[0], NULL, NULL, true, NULL, triggeredByAura);
                    return SPELL_AURA_PROC_OK;
                }
                // Judgement of Wisdom
                case 20186:
                {
                    if (pVictim->getPowerType() == POWER_MANA)
                    {
                        // 2% of maximum base mana
                        basepoints[0] = int32(pVictim->GetCreateMana() * 2 / 100);
                        pVictim->CastCustomSpell(pVictim, 20268, &basepoints[0], NULL, NULL, true, NULL, triggeredByAura);
                    }
                    return SPELL_AURA_PROC_OK;
                }
                // Heart of the Crusader (Rank 1)
                case 20335:
                    triggered_spell_id = 21183;
                    break;
                // Heart of the Crusader (Rank 2)
                case 20336:
                    triggered_spell_id = 54498;
                    break;
                // Heart of the Crusader (Rank 3)
                case 20337:
                    triggered_spell_id = 54499;
                    break;
                case 20911:                                 // Blessing of Sanctuary
                case 25899:                                 // Greater Blessing of Sanctuary
                {
                    target = this;
                    switch (target->getPowerType())
                    {
                        case POWER_MANA:
                            triggered_spell_id = 57319;
                            break;
                        default:
                            return SPELL_AURA_PROC_FAILED;
                    }
                    break;
                }
                // Holy Power (Redemption Armor set)
                case 28789:
                {
                    if(!pVictim)
                        return SPELL_AURA_PROC_FAILED;

                    // Set class defined buff
                    switch (pVictim->getClass())
                    {
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                        case CLASS_SHAMAN:
                        case CLASS_DRUID:
                            triggered_spell_id = 28795;     // Increases the friendly target's mana regeneration by $s1 per 5 sec. for $d.
                            break;
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                            triggered_spell_id = 28793;     // Increases the friendly target's spell damage and healing by up to $s1 for $d.
                            break;
                        case CLASS_HUNTER:
                        case CLASS_ROGUE:
                            triggered_spell_id = 28791;     // Increases the friendly target's attack power by $s1 for $d.
                            break;
                        case CLASS_WARRIOR:
                            triggered_spell_id = 28790;     // Increases the friendly target's armor
                            break;
                        default:
                            return SPELL_AURA_PROC_FAILED;
                    }
                    break;
                }
                // Spiritual Attunement
                case 31785:
                case 33776:
                {
                    // if healed by another unit (pVictim)
                    if (this == pVictim)
                        return SPELL_AURA_PROC_FAILED;

                    // heal amount
                    basepoints[0] = triggerAmount*damage/100;
                    target = this;
                    triggered_spell_id = 31786;
                    break;
                }
                // Seal of Vengeance (damage calc on apply aura)
                case 31801:
                {
                    if (effIndex != EFFECT_INDEX_0)         // effect 1,2 used by seal unleashing code
                        return SPELL_AURA_PROC_FAILED;

                    // At melee attack or Hammer of the Righteous spell damage considered as melee attack
                    if ((procFlag & PROC_FLAG_SUCCESSFUL_MELEE_HIT) || (procSpell && procSpell->Id == 53595) )
                        triggered_spell_id = 31803;         // Holy Vengeance

                    // Add 5-stack effect from Holy Vengeance
                    int8 stacks = 0;
                    AuraList const& auras = target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for(AuraList::const_iterator itr = auras.begin(); itr!=auras.end(); ++itr)
                    {
                        if( ((*itr)->GetId() == 31803) && (*itr)->GetCasterGUID()==GetGUID())
                        {
                            stacks = (*itr)->GetStackAmount();
                            break;
                        }
                    }
                    if(stacks >= 5)
                        CastSpell(target,42463,true,NULL,triggeredByAura);
                    break;
                }
                // Judgements of the Wise
                case 31876:
                case 31877:
                case 31878:
                    // triggered only at casted Judgement spells, not at additional Judgement effects
                    if(!procSpell || procSpell->Category != 1210)
                        return SPELL_AURA_PROC_FAILED;

                    target = this;
                    triggered_spell_id = 31930;

                    // Replenishment
                    CastSpell(this, 57669, true, NULL, triggeredByAura);
                    break;
                // Paladin Tier 6 Trinket (Ashtongue Talisman of Zeal)
                case 40470:
                {
                    if (!procSpell)
                        return SPELL_AURA_PROC_FAILED;

                    float  chance;

                    // Flash of light/Holy light
                    if (procSpell->SpellFamilyFlags & UI64LIT(0x00000000C0000000))
                    {
                        triggered_spell_id = 40471;
                        chance = 15.0f;
                    }
                    // Judgement (any)
                    else if (GetSpellSpecific(procSpell->Id)==SPELL_JUDGEMENT)
                    {
                        triggered_spell_id = 40472;
                        chance = 50.0f;
                    }
                    else
                        return SPELL_AURA_PROC_FAILED;

                    if (!roll_chance_f(chance))
                        return SPELL_AURA_PROC_FAILED;

                    break;
                }
                // Light's Beacon (heal target area aura)
                case 53651:
                {
                    // not do bonus heal for explicit beacon focus healing
                    if (GetGUID() == triggeredByAura->GetCasterGUID())
                        return SPELL_AURA_PROC_FAILED;

                    // beacon
                    Unit* beacon = triggeredByAura->GetCaster();
                    if (!beacon)
                        return SPELL_AURA_PROC_FAILED;

                    // find caster main aura at beacon
                    Aura* dummy = NULL;
                    Unit::AuraList const& baa = beacon->GetAurasByType(SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                    for(Unit::AuraList::const_iterator i = baa.begin(); i != baa.end(); ++i)
                    {
                        if ((*i)->GetId() == 53563 && (*i)->GetCasterGUID() == pVictim->GetGUID())
                        {
                            dummy = (*i);
                            break;
                        }
                    }

                    // original heal must be form beacon caster
                    if (!dummy)
                        return SPELL_AURA_PROC_FAILED;

                    triggered_spell_id = 53652;             // Beacon of Light
                    basepoints[0] = triggeredByAura->GetModifier()->m_amount*damage/100;

                    // cast with original caster set but beacon to beacon for apply caster mods and avoid LoS check
                    beacon->CastCustomSpell(beacon,triggered_spell_id,&basepoints[0],NULL,NULL,true,castItem,triggeredByAura,pVictim->GetGUID());
                    return SPELL_AURA_PROC_OK;
                }
                // Seal of Corruption (damage calc on apply aura)
                case 53736:
                {
                    if (effIndex != EFFECT_INDEX_0)         // effect 1,2 used by seal unleashing code
                        return SPELL_AURA_PROC_FAILED;

                    // At melee attack or Hammer of the Righteous spell damage considered as melee attack
                    if ((procFlag & PROC_FLAG_SUCCESSFUL_MELEE_HIT) || (procSpell && procSpell->Id == 53595))
                        triggered_spell_id = 53742;         // Blood Corruption

                    // Add 5-stack effect from Blood Corruption
                    int8 stacks = 0;
                    AuraList const& auras = target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for(AuraList::const_iterator itr = auras.begin(); itr!=auras.end(); ++itr)
                    {
                        if( ((*itr)->GetId() == 53742) && (*itr)->GetCasterGUID()==GetGUID())
                        {
                            stacks = (*itr)->GetStackAmount();
                            break;
                        }
                    }
                    if(stacks >= 5)
                        CastSpell(target,53739,true,NULL,triggeredByAura);
                    break;
                }
                // Glyph of Holy Light
                case 54937:
                {
                    triggered_spell_id = 54968;
                    basepoints[0] = triggerAmount*damage/100;
                    break;
                }
                // Sacred Shield (buff)
                case 58597:
                {
                    triggered_spell_id = 66922;
                    SpellEntry const* triggeredEntry = sSpellStore.LookupEntry(triggered_spell_id);
                    if (!triggeredEntry)
                        return SPELL_AURA_PROC_FAILED;

                    basepoints[0] = int32(damage / (GetSpellDuration(triggeredEntry) / triggeredEntry->EffectAmplitude[EFFECT_INDEX_0]));
                    target = this;
                    break;
                }
                // Sacred Shield (talent rank)
                case 53601:
                {
                    // triggered_spell_id in spell data
                    target = this;
                    break;
                }
                // Anger Capacitor
                case 71406:                                 // normal
                case 71545:                                 // heroic
                {
                    if (!pVictim)
                        return SPELL_AURA_PROC_FAILED;

                    SpellEntry const* mote = sSpellStore.LookupEntry(71432);
                    if (!mote)
                        return SPELL_AURA_PROC_FAILED;
                    uint32 maxStack = mote->StackAmount - (dummySpell->Id == 71545 ? 1 : 0);

                    SpellAuraHolder *aurHolder = GetSpellAuraHolder(71432);
                    if (aurHolder && uint32(aurHolder->GetStackAmount() +1) >= maxStack)
                    {
                        RemoveAurasDueToSpell(71432);       // Mote of Anger

                        // Manifest Anger (main hand/off hand)
                        CastSpell(pVictim, !haveOffhandWeapon() || roll_chance_i(50) ? 71433 : 71434, true);
                        return SPELL_AURA_PROC_OK;
                    }
                    else
                        triggered_spell_id = 71432;

                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            switch(dummySpell->Id)
            {
                // Totemic Power (The Earthshatterer set)
                case 28823:
                {
                    if( !pVictim )
                        return SPELL_AURA_PROC_FAILED;

                    // Set class defined buff
                    switch (pVictim->getClass())
                    {
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                        case CLASS_SHAMAN:
                        case CLASS_DRUID:
                            triggered_spell_id = 28824;     // Increases the friendly target's mana regeneration by $s1 per 5 sec. for $d.
                            break;
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                            triggered_spell_id = 28825;     // Increases the friendly target's spell damage and healing by up to $s1 for $d.
                            break;
                        case CLASS_HUNTER:
                        case CLASS_ROGUE:
                            triggered_spell_id = 28826;     // Increases the friendly target's attack power by $s1 for $d.
                            break;
                        case CLASS_WARRIOR:
                            triggered_spell_id = 28827;     // Increases the friendly target's armor
                            break;
                        default:
                            return SPELL_AURA_PROC_FAILED;
                    }
                    break;
                }
                // Lesser Healing Wave (Totem of Flowing Water Relic)
                case 28849:
                {
                    target = this;
                    triggered_spell_id = 28850;
                    break;
                }
                // Windfury Weapon (Passive) 1-5 Ranks
                case 33757:
                {
                    if(GetTypeId()!=TYPEID_PLAYER)
                        return SPELL_AURA_PROC_FAILED;

                    if(!castItem || !castItem->IsEquipped())
                        return SPELL_AURA_PROC_FAILED;

                    // custom cooldown processing case
                    if( cooldown && ((Player*)this)->HasSpellCooldown(dummySpell->Id))
                        return SPELL_AURA_PROC_FAILED;

                    // Now amount of extra power stored in 1 effect of Enchant spell
                    // Get it by item enchant id
                    uint32 spellId;
                    switch (castItem->GetEnchantmentId(EnchantmentSlot(TEMP_ENCHANTMENT_SLOT)))
                    {
                        case 283: spellId =  8232; break;   // 1 Rank
                        case 284: spellId =  8235; break;   // 2 Rank
                        case 525: spellId = 10486; break;   // 3 Rank
                        case 1669:spellId = 16362; break;   // 4 Rank
                        case 2636:spellId = 25505; break;   // 5 Rank
                        case 3785:spellId = 58801; break;   // 6 Rank
                        case 3786:spellId = 58803; break;   // 7 Rank
                        case 3787:spellId = 58804; break;   // 8 Rank
                        default:
                        {
                            sLog.outError("Unit::HandleDummyAuraProc: non handled item enchantment (rank?) %u for spell id: %u (Windfury)",
                                castItem->GetEnchantmentId(EnchantmentSlot(TEMP_ENCHANTMENT_SLOT)),dummySpell->Id);
                            return SPELL_AURA_PROC_FAILED;
                        }
                    }

                    SpellEntry const* windfurySpellEntry = sSpellStore.LookupEntry(spellId);
                    if(!windfurySpellEntry)
                    {
                        sLog.outError("Unit::HandleDummyAuraProc: nonexistent spell id: %u (Windfury)",spellId);
                        return SPELL_AURA_PROC_FAILED;
                    }

                    int32 extra_attack_power = CalculateSpellDamage(pVictim, windfurySpellEntry, EFFECT_INDEX_1);

                    // Off-Hand case
                    if (castItem->GetSlot() == EQUIPMENT_SLOT_OFFHAND)
                    {
                        // Value gained from additional AP
                        basepoints[0] = int32(extra_attack_power/14.0f * GetAttackTime(OFF_ATTACK)/1000/2);
                        triggered_spell_id = 33750;
                    }
                    // Main-Hand case
                    else
                    {
                        // Value gained from additional AP
                        basepoints[0] = int32(extra_attack_power/14.0f * GetAttackTime(BASE_ATTACK)/1000);
                        triggered_spell_id = 25504;
                    }

                    // apply cooldown before cast to prevent processing itself
                    if( cooldown )
                        ((Player*)this)->AddSpellCooldown(dummySpell->Id,0,time(NULL) + cooldown);

                    // Attack Twice
                    for ( uint32 i = 0; i<2; ++i )
                        CastCustomSpell(pVictim,triggered_spell_id,&basepoints[0],NULL,NULL,true,castItem,triggeredByAura);

                    return SPELL_AURA_PROC_OK;
                }
                // Shaman Tier 6 Trinket
                case 40463:
                {
                    if( !procSpell )
                        return SPELL_AURA_PROC_FAILED;

                    float  chance;
                    if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000001))
                    {
                        triggered_spell_id = 40465;         // Lightning Bolt
                        chance = 15.0f;
                    }
                    else if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000080))
                    {
                        triggered_spell_id = 40465;         // Lesser Healing Wave
                        chance = 10.0f;
                    }
                    else if (procSpell->SpellFamilyFlags & UI64LIT(0x0000001000000000))
                    {
                        triggered_spell_id = 40466;         // Stormstrike
                        chance = 50.0f;
                    }
                    else
                        return SPELL_AURA_PROC_FAILED;

                    if (!roll_chance_f(chance))
                        return SPELL_AURA_PROC_FAILED;

                    target = this;
                    break;
                }
                // Glyph of Healing Wave
                case 55440:
                {
                    // Not proc from self heals
                    if (this==pVictim)
                        return SPELL_AURA_PROC_FAILED;
                    basepoints[0] = triggerAmount * damage / 100;
                    target = this;
                    triggered_spell_id = 55533;
                    break;
                }
                // Spirit Hunt
                case 58877:
                {
                    // Cast on owner
                    target = GetOwner();
                    if (!target)
                        return SPELL_AURA_PROC_FAILED;
                    basepoints[0] = triggerAmount * damage / 100;
                    triggered_spell_id = 58879;
                    break;
                }
                // Glyph of Totem of Wrath
                case 63280:
                {
                    Totem* totem = GetTotem(TOTEM_SLOT_FIRE);
                    if (!totem)
                        return SPELL_AURA_PROC_FAILED;

                    // find totem aura bonus
                    AuraList const& spellPower = totem->GetAurasByType(SPELL_AURA_NONE);
                    for(AuraList::const_iterator i = spellPower.begin();i != spellPower.end(); ++i)
                    {
                        // select proper aura for format aura type in spell proto
                        if ((*i)->GetTarget()==totem && (*i)->GetSpellProto()->EffectApplyAuraName[(*i)->GetEffIndex()] == SPELL_AURA_MOD_HEALING_DONE &&
                            (*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_SHAMAN && (*i)->GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000004000000))
                        {
                            basepoints[0] = triggerAmount * (*i)->GetModifier()->m_amount / 100;
                            break;
                        }
                    }

                    if (!basepoints[0])
                        return SPELL_AURA_PROC_FAILED;

                    basepoints[1] = basepoints[0];
                    triggered_spell_id = 63283;             // Totem of Wrath, caster bonus
                    target = this;
                    break;
                }
                // Shaman T8 Elemental 4P Bonus
                case 64928:
                {
                    basepoints[0] = int32( triggerAmount * damage / 100 );
                    triggered_spell_id = 64930;            // Electrified
                    break;
                }
                // Shaman T9 Elemental 4P Bonus
                case 67228:
                {
                    basepoints[0] = int32( triggerAmount * damage / 100 );
                    triggered_spell_id = 71824;
                    break;
                }
            }
            // Storm, Earth and Fire
            if (dummySpell->SpellIconID == 3063)
            {
                // Earthbind Totem summon only
                if(procSpell->Id != 2484)
                    return SPELL_AURA_PROC_FAILED;

                if (!roll_chance_i(triggerAmount))
                    return SPELL_AURA_PROC_FAILED;

                triggered_spell_id = 64695;
                break;
            }
            // Ancestral Awakening
            if (dummySpell->SpellIconID == 3065)
            {
                triggered_spell_id = 52759;
                basepoints[0] = triggerAmount * damage / 100;
                target = this;
                break;
            }
            // Flametongue Weapon (Passive), Ranks
            if (dummySpell->SpellFamilyFlags & UI64LIT(0x0000000000200000))
            {
                if (GetTypeId()!=TYPEID_PLAYER || !castItem)
                    return SPELL_AURA_PROC_FAILED;

                // Only proc for enchanted weapon
                Item *usedWeapon = ((Player *)this)->GetWeaponForAttack(procFlag & PROC_FLAG_SUCCESSFUL_OFFHAND_HIT ? OFF_ATTACK : BASE_ATTACK, true, true);
                if (usedWeapon != castItem)
                    return SPELL_AURA_PROC_FAILED;

                switch (dummySpell->Id)
                {
                    case 10400: triggered_spell_id =  8026; break; // Rank 1
                    case 15567: triggered_spell_id =  8028; break; // Rank 2
                    case 15568: triggered_spell_id =  8029; break; // Rank 3
                    case 15569: triggered_spell_id = 10445; break; // Rank 4
                    case 16311: triggered_spell_id = 16343; break; // Rank 5
                    case 16312: triggered_spell_id = 16344; break; // Rank 6
                    case 16313: triggered_spell_id = 25488; break; // Rank 7
                    case 58784: triggered_spell_id = 58786; break; // Rank 8
                    case 58791: triggered_spell_id = 58787; break; // Rank 9
                    case 58792: triggered_spell_id = 58788; break; // Rank 10
                    default:
                        return SPELL_AURA_PROC_FAILED;
                }
                break;
            }
            // Earth Shield
            if (dummySpell->SpellFamilyFlags & UI64LIT(0x0000040000000000))
            {
                target = this;
                basepoints[0] = triggerAmount;

                // Glyph of Earth Shield
                if (Aura* aur = GetDummyAura(63279))
                {
                    int32 aur_mod = aur->GetModifier()->m_amount;
                    basepoints[0] = int32(basepoints[0] * (aur_mod + 100.0f) / 100.0f);
                }

                triggered_spell_id = 379;
                break;
            }
            // Improved Water Shield
            if (dummySpell->SpellIconID == 2287)
            {
                // Lesser Healing Wave need aditional 60% roll
                if ((procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000080)) && !roll_chance_i(60))
                    return SPELL_AURA_PROC_FAILED;
                // Chain Heal needs additional 30% roll
                if ((procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000100)) && !roll_chance_i(30))
                    return SPELL_AURA_PROC_FAILED;
                // lookup water shield
                AuraList const& vs = GetAurasByType(SPELL_AURA_PROC_TRIGGER_SPELL);
                for(AuraList::const_iterator itr = vs.begin(); itr != vs.end(); ++itr)
                {
                    if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_SHAMAN &&
                        ((*itr)->GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000002000000000)))
                    {
                        uint32 spell = (*itr)->GetSpellProto()->EffectTriggerSpell[(*itr)->GetEffIndex()];
                        CastSpell(this, spell, true, castItem, triggeredByAura);
                        return SPELL_AURA_PROC_OK;
                    }
                }
                return SPELL_AURA_PROC_FAILED;
            }
            // Lightning Overload
            if (dummySpell->SpellIconID == 2018)            // only this spell have SpellFamily Shaman SpellIconID == 2018 and dummy aura
            {
                if(!procSpell || GetTypeId() != TYPEID_PLAYER || !pVictim )
                    return SPELL_AURA_PROC_FAILED;

                // custom cooldown processing case
                if( cooldown && GetTypeId()==TYPEID_PLAYER && ((Player*)this)->HasSpellCooldown(dummySpell->Id))
                    return SPELL_AURA_PROC_FAILED;

                uint32 spellId = 0;
                // Every Lightning Bolt and Chain Lightning spell have duplicate vs half damage and zero cost
                switch (procSpell->Id)
                {
                    // Lightning Bolt
                    case   403: spellId = 45284; break;     // Rank  1
                    case   529: spellId = 45286; break;     // Rank  2
                    case   548: spellId = 45287; break;     // Rank  3
                    case   915: spellId = 45288; break;     // Rank  4
                    case   943: spellId = 45289; break;     // Rank  5
                    case  6041: spellId = 45290; break;     // Rank  6
                    case 10391: spellId = 45291; break;     // Rank  7
                    case 10392: spellId = 45292; break;     // Rank  8
                    case 15207: spellId = 45293; break;     // Rank  9
                    case 15208: spellId = 45294; break;     // Rank 10
                    case 25448: spellId = 45295; break;     // Rank 11
                    case 25449: spellId = 45296; break;     // Rank 12
                    case 49237: spellId = 49239; break;     // Rank 13
                    case 49238: spellId = 49240; break;     // Rank 14
                    // Chain Lightning
                    case   421: spellId = 45297; break;     // Rank  1
                    case   930: spellId = 45298; break;     // Rank  2
                    case  2860: spellId = 45299; break;     // Rank  3
                    case 10605: spellId = 45300; break;     // Rank  4
                    case 25439: spellId = 45301; break;     // Rank  5
                    case 25442: spellId = 45302; break;     // Rank  6
                    case 49270: spellId = 49268; break;     // Rank  7
                    case 49271: spellId = 49269; break;     // Rank  8
                    default:
                        sLog.outError("Unit::HandleDummyAuraProc: non handled spell id: %u (LO)", procSpell->Id);
                        return SPELL_AURA_PROC_FAILED;
                }
                // No thread generated mod
                // TODO: exist special flag in spell attributes for this, need found and use!
                SpellModifier *mod = new SpellModifier(SPELLMOD_THREAT,SPELLMOD_PCT,-100,triggeredByAura);

                ((Player*)this)->AddSpellMod(mod, true);

                // Remove cooldown (Chain Lightning - have Category Recovery time)
                if (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000002))
                    ((Player*)this)->RemoveSpellCooldown(spellId);

                CastSpell(pVictim, spellId, true, castItem, triggeredByAura);

                ((Player*)this)->AddSpellMod(mod, false);

                if( cooldown && GetTypeId()==TYPEID_PLAYER )
                    ((Player*)this)->AddSpellCooldown(dummySpell->Id,0,time(NULL) + cooldown);

                return SPELL_AURA_PROC_OK;
            }
            // Static Shock
            if(dummySpell->SpellIconID == 3059)
            {
                // lookup Lightning Shield
                AuraList const& vs = GetAurasByType(SPELL_AURA_PROC_TRIGGER_SPELL);
                for(AuraList::const_iterator itr = vs.begin(); itr != vs.end(); ++itr)
                {
                    if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_SHAMAN &&
                        ((*itr)->GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000400)))
                    {
                        uint32 spell = 0;
                        switch ((*itr)->GetId())
                        {
                            case   324: spell = 26364; break;
                            case   325: spell = 26365; break;
                            case   905: spell = 26366; break;
                            case   945: spell = 26367; break;
                            case  8134: spell = 26369; break;
                            case 10431: spell = 26370; break;
                            case 10432: spell = 26363; break;
                            case 25469: spell = 26371; break;
                            case 25472: spell = 26372; break;
                            case 49280: spell = 49278; break;
                            case 49281: spell = 49279; break;
                            default:
                                return SPELL_AURA_PROC_FAILED;
                        }
                        CastSpell(target, spell, true, castItem, triggeredByAura);
                        if ((*itr)->GetHolder()->DropAuraCharge())
                            RemoveAuraHolderFromStack((*itr)->GetId());
                        return SPELL_AURA_PROC_OK;
                    }
                }
                return SPELL_AURA_PROC_FAILED;
            }
            // Frozen Power
            if (dummySpell->SpellIconID == 3780)
            {
                Unit *caster = triggeredByAura->GetCaster();

                if (!procSpell || !caster)
                    return SPELL_AURA_PROC_FAILED;

                float distance = caster->GetDistance(pVictim);
                int32 chance = triggerAmount;

                if (distance < 15.0f || !roll_chance_i(chance))
                    return SPELL_AURA_PROC_FAILED;

                // make triggered cast apply after current damage spell processing for prevent remove by it
                if(Spell* spell = GetCurrentSpell(CURRENT_GENERIC_SPELL))
                    spell->AddTriggeredSpell(63685);
                return SPELL_AURA_PROC_OK;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Butchery
            if (dummySpell->SpellIconID == 2664)
            {
                basepoints[0] = triggerAmount;
                triggered_spell_id = 50163;
                target = this;
                break;
            }
            // Dancing Rune Weapon
            if (dummySpell->Id == 49028)
            {
                // 1 dummy aura for dismiss rune blade
                if (effIndex != EFFECT_INDEX_2)
                    return SPELL_AURA_PROC_FAILED;
                // TODO: wite script for this "fights on its own, doing the same attacks"
                // NOTE: Trigger here on every attack and spell cast
                return SPELL_AURA_PROC_FAILED;
            }
            // Mark of Blood
            if (dummySpell->Id == 49005)
            {
                // TODO: need more info (cooldowns/PPM)
                triggered_spell_id = 61607;
                break;
            }
            // Vendetta
            if (dummySpell->SpellFamilyFlags & UI64LIT(0x0000000000010000))
            {
                basepoints[0] = triggerAmount * GetMaxHealth() / 100;
                triggered_spell_id = 50181;
                target = this;
                break;
            }
            // Necrosis
            if (dummySpell->SpellIconID == 2709)
            {
                // only melee auto attack affected and Rune Strike
                if (procSpell && procSpell->Id != 56815)
                    return SPELL_AURA_PROC_FAILED;

                basepoints[0] = triggerAmount * damage / 100;
                triggered_spell_id = 51460;
                break;
            }
            // Threat of Thassarian
            if (dummySpell->SpellIconID == 2023)
            {
                // Must Dual Wield
                if (!procSpell || !haveOffhandWeapon())
                    return SPELL_AURA_PROC_FAILED;
                // Chance as basepoints for dummy aura
                if (!roll_chance_i(triggerAmount))
                    return SPELL_AURA_PROC_FAILED;

                switch (procSpell->Id)
                {
                    // Obliterate
                    case 49020:                             // Rank 1
                        triggered_spell_id = 66198; break;
                    case 51423:                             // Rank 2
                        triggered_spell_id = 66972; break;
                    case 51424:                             // Rank 3
                        triggered_spell_id = 66973; break;
                    case 51425:                             // Rank 4
                        triggered_spell_id = 66974; break;
                    // Frost Strike
                    case 49143:                             // Rank 1
                        triggered_spell_id = 66196; break;
                    case 51416:                             // Rank 2
                        triggered_spell_id = 66958; break;
                    case 51417:                             // Rank 3
                        triggered_spell_id = 66959; break;
                    case 51418:                             // Rank 4
                        triggered_spell_id = 66960; break;
                    case 51419:                             // Rank 5
                        triggered_spell_id = 66961; break;
                    case 55268:                             // Rank 6
                        triggered_spell_id = 66962; break;
                    // Plague Strike
                    case 45462:                             // Rank 1
                        triggered_spell_id = 66216; break;
                    case 49917:                             // Rank 2
                        triggered_spell_id = 66988; break;
                    case 49918:                             // Rank 3
                        triggered_spell_id = 66989; break;
                    case 49919:                             // Rank 4
                        triggered_spell_id = 66990; break;
                    case 49920:                             // Rank 5
                        triggered_spell_id = 66991; break;
                    case 49921:                             // Rank 6
                        triggered_spell_id = 66992; break;
                    // Death Strike
                    case 49998:                             // Rank 1
                        triggered_spell_id = 66188; break;
                    case 49999:                             // Rank 2
                        triggered_spell_id = 66950; break;
                    case 45463:                             // Rank 3
                        triggered_spell_id = 66951; break;
                    case 49923:                             // Rank 4
                        triggered_spell_id = 66952; break;
                    case 49924:                             // Rank 5
                        triggered_spell_id = 66953; break;
                    // Rune Strike
                    case 56815:
                        triggered_spell_id = 66217; break;
                    // Blood Strike
                    case 45902:                             // Rank 1
                        triggered_spell_id = 66215; break;
                    case 49926:                             // Rank 2
                        triggered_spell_id = 66975; break;
                    case 49927:                             // Rank 3
                        triggered_spell_id = 66976; break;
                    case 49928:                             // Rank 4
                        triggered_spell_id = 66977; break;
                    case 49929:                             // Rank 5
                        triggered_spell_id = 66978; break;
                    case 49930:                             // Rank 6
                        triggered_spell_id = 66979; break;
                    default:
                        return SPELL_AURA_PROC_FAILED;
                }
                break;
            }
            // Runic Power Back on Snare/Root
            if (dummySpell->Id == 61257)
            {
                // only for spells and hit/crit (trigger start always) and not start from self casted spells
                if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == pVictim)
                    return SPELL_AURA_PROC_FAILED;
                // Need snare or root mechanic
                if (!(GetAllSpellMechanicMask(procSpell) & IMMUNE_TO_ROOT_AND_SNARE_MASK))
                    return SPELL_AURA_PROC_FAILED;
                triggered_spell_id = 61258;
                target = this;
                break;
            }
            // Wandering Plague
            if (dummySpell->SpellIconID == 1614)
            {
                if (!roll_chance_f(GetUnitCriticalChance(BASE_ATTACK, pVictim)))
                    return SPELL_AURA_PROC_FAILED;
                basepoints[0] = triggerAmount * damage / 100;
                triggered_spell_id = 50526;
                break;
            }
            // Blood-Caked Blade
            if (dummySpell->SpellIconID == 138)
            {
                // only main hand melee auto attack affected and Rune Strike
                if ((procFlag & PROC_FLAG_SUCCESSFUL_OFFHAND_HIT) || procSpell && procSpell->Id != 56815)
                    return SPELL_AURA_PROC_FAILED;

                // triggered_spell_id in spell data
                break;
            }
            break;
        }
        default:
            break;
    }

    // processed charge only counting case
    if(!triggered_spell_id)
        return SPELL_AURA_PROC_OK;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if(!triggerEntry)
    {
        sLog.outError("Unit::HandleDummyAuraProc: Spell %u have nonexistent triggered spell %u",dummySpell->Id,triggered_spell_id);
        return SPELL_AURA_PROC_FAILED;
    }

    // default case
    if(!target || target!=this && !target->isAlive())
        return SPELL_AURA_PROC_FAILED;

    if( cooldown && GetTypeId()==TYPEID_PLAYER && ((Player*)this)->HasSpellCooldown(triggered_spell_id))
        return SPELL_AURA_PROC_FAILED;

    if (basepoints[EFFECT_INDEX_0] || basepoints[EFFECT_INDEX_1] || basepoints[EFFECT_INDEX_2])
        CastCustomSpell(target, triggered_spell_id,
            basepoints[EFFECT_INDEX_0] ? &basepoints[EFFECT_INDEX_0] : NULL,
            basepoints[EFFECT_INDEX_1] ? &basepoints[EFFECT_INDEX_1] : NULL,
            basepoints[EFFECT_INDEX_2] ? &basepoints[EFFECT_INDEX_2] : NULL,
            true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (cooldown && GetTypeId()==TYPEID_PLAYER)
        ((Player*)this)->AddSpellCooldown(triggered_spell_id,0,time(NULL) + cooldown);

    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleProcTriggerSpellAuraProc(Unit *pVictim, uint32 damage, Aura* triggeredByAura, SpellEntry const *procSpell, uint32 procFlags, uint32 procEx, uint32 cooldown)
{
    // Get triggered aura spell info
    SpellEntry const* auraSpellInfo = triggeredByAura->GetSpellProto();

    // Basepoints of trigger aura
    int32 triggerAmount = triggeredByAura->GetModifier()->m_amount;

    // Set trigger spell id, target, custom basepoints
    uint32 trigger_spell_id = auraSpellInfo->EffectTriggerSpell[triggeredByAura->GetEffIndex()];
    Unit*  target = NULL;
    int32  basepoints[MAX_EFFECT_INDEX] = {0, 0, 0};

    if(triggeredByAura->GetModifier()->m_auraname == SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE)
        basepoints[0] = triggerAmount;

    Item* castItem = triggeredByAura->GetCastItemGUID() && GetTypeId()==TYPEID_PLAYER
        ? ((Player*)this)->GetItemByGuid(triggeredByAura->GetCastItemGUID()) : NULL;

    // Try handle unknown trigger spells
    // Custom requirements (not listed in procEx) Warning! damage dealing after this
    // Custom triggered spells
    switch (auraSpellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch(auraSpellInfo->Id)
            {
                //case 191:                               // Elemental Response
                //    switch (procSpell->School)
                //    {
                //        case SPELL_SCHOOL_FIRE:  trigger_spell_id = 34192; break;
                //        case SPELL_SCHOOL_FROST: trigger_spell_id = 34193; break;
                //        case SPELL_SCHOOL_ARCANE:trigger_spell_id = 34194; break;
                //        case SPELL_SCHOOL_NATURE:trigger_spell_id = 34195; break;
                //        case SPELL_SCHOOL_SHADOW:trigger_spell_id = 34196; break;
                //        case SPELL_SCHOOL_HOLY:  trigger_spell_id = 34197; break;
                //        case SPELL_SCHOOL_NORMAL:trigger_spell_id = 34198; break;
                //    }
                //    break;
                //case 5301:  break;                        // Defensive State (DND)
                //case 7137:  break:                        // Shadow Charge (Rank 1)
                //case 7377:  break:                        // Take Immune Periodic Damage <Not Working>
                //case 13358: break;                        // Defensive State (DND)
                //case 16092: break;                        // Defensive State (DND)
                //case 18943: break;                        // Double Attack
                //case 19194: break;                        // Double Attack
                //case 19817: break;                        // Double Attack
                //case 19818: break;                        // Double Attack
                //case 22835: break;                        // Drunken Rage
                //    trigger_spell_id = 14822; break;
                case 23780:                                 // Aegis of Preservation (Aegis of Preservation trinket)
                    trigger_spell_id = 23781;
                    break;
                //case 24949: break;                        // Defensive State 2 (DND)
                case 27522:                                 // Mana Drain Trigger
                case 40336:                                 // Mana Drain Trigger
                    // On successful melee or ranged attack gain $29471s1 mana and if possible drain $27526s1 mana from the target.
                    if (isAlive())
                        CastSpell(this, 29471, true, castItem, triggeredByAura);
                    if (pVictim && pVictim->isAlive())
                        CastSpell(pVictim, 27526, true, castItem, triggeredByAura);
                    return SPELL_AURA_PROC_OK;
                case 31255:                                 // Deadly Swiftness (Rank 1)
                    // whenever you deal damage to a target who is below 20% health.
                    if (pVictim->GetHealth() > pVictim->GetMaxHealth() / 5)
                        return SPELL_AURA_PROC_FAILED;

                    target = this;
                    trigger_spell_id = 22588;
                    break;
                //case 33207: break;                        // Gossip NPC Periodic - Fidget
                case 33896:                                 // Desperate Defense (Stonescythe Whelp, Stonescythe Alpha, Stonescythe Ambusher)
                    trigger_spell_id = 33898;
                    break;
                //case 34082: break;                        // Advantaged State (DND)
                //case 34783: break:                        // Spell Reflection
                //case 35205: break:                        // Vanish
                //case 35321: break;                        // Gushing Wound
                //case 36096: break:                        // Spell Reflection
                //case 36207: break:                        // Steal Weapon
                //case 36576: break:                        // Shaleskin (Shaleskin Flayer, Shaleskin Ripper) 30023 trigger
                //case 37030: break;                        // Chaotic Temperament
                //case 38363: break;                        // Gushing Wound
                //case 39215: break;                        // Gushing Wound
                //case 40250: break;                        // Improved Duration
                //case 40329: break;                        // Demo Shout Sensor
                //case 40364: break;                        // Entangling Roots Sensor
                //case 41054: break;                        // Copy Weapon
                //    trigger_spell_id = 41055; break;
                //case 41248: break;                        // Consuming Strikes
                //    trigger_spell_id = 41249; break;
                //case 42730: break:                        // Woe Strike
                //case 43453: break:                        // Rune Ward
                //case 43504: break;                        // Alterac Valley OnKill Proc Aura
                //case 44326: break:                        // Pure Energy Passive
                //case 44526: break;                        // Hate Monster (Spar) (30 sec)
                //case 44527: break;                        // Hate Monster (Spar Buddy) (30 sec)
                //case 44819: break;                        // Hate Monster (Spar Buddy) (>30% Health)
                //case 44820: break;                        // Hate Monster (Spar) (<30%)
                case 45057:                                 // Evasive Maneuvers (Commendation of Kael`thas trinket)
                    // reduce you below $s1% health
                    if (GetHealth() - damage > GetMaxHealth() * triggerAmount / 100)
                        return SPELL_AURA_PROC_FAILED;
                    break;
                //case 45903: break:                        // Offensive State
                //case 46146: break:                        // [PH] Ahune  Spanky Hands
                //case 46939: break;                        // Black Bow of the Betrayer
                //    trigger_spell_id = 29471; - gain mana
                //                       27526; - drain mana if possible
                case 43820:                                 // Charm of the Witch Doctor (Amani Charm of the Witch Doctor trinket)
                    // Pct value stored in dummy
                    basepoints[0] = pVictim->GetCreateHealth() * auraSpellInfo->CalculateSimpleValue(EFFECT_INDEX_1) / 100;
                    target = pVictim;
                    break;
                //case 45205: break;                        // Copy Offhand Weapon
                //case 45343: break;                        // Dark Flame Aura
                //case 47300: break;                        // Dark Flame Aura
                //case 48876: break;                        // Beast's Mark
                //    trigger_spell_id = 48877; break;
                //case 49059: break;                        // Horde, Hate Monster (Spar Buddy) (>30% Health)
                //case 50051: break;                        // Ethereal Pet Aura
                //case 50689: break;                        // Blood Presence (Rank 1)
                //case 50844: break;                        // Blood Mirror
                //case 52856: break;                        // Charge
                //case 54072: break;                        // Knockback Ball Passive
                //case 54476: break;                        // Blood Presence
                //case 54775: break;                        // Abandon Vehicle on Poly
                case 57345:                                 // Darkmoon Card: Greatness
                {
                    float stat = 0.0f;
                    // strength
                    if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 60229;stat = GetStat(STAT_STRENGTH); }
                    // agility
                    if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 60233;stat = GetStat(STAT_AGILITY);  }
                    // intellect
                    if (GetStat(STAT_INTELLECT)> stat) { trigger_spell_id = 60234;stat = GetStat(STAT_INTELLECT);}
                    // spirit
                    if (GetStat(STAT_SPIRIT)   > stat) { trigger_spell_id = 60235;                               }
                    break;
                }
                //case 55580: break:                        // Mana Link
                //case 57587: break:                        // Steal Ranged ()
                //case 57594: break;                        // Copy Ranged Weapon
                //case 59237: break;                        // Beast's Mark
                //    trigger_spell_id = 59233; break;
                //case 59288: break;                        // Infra-Green Shield
                //case 59532: break;                        // Abandon Passengers on Poly
                //case 59735: break:                        // Woe Strike
                case 64415:                                 // // Val'anyr Hammer of Ancient Kings - Equip Effect
                {
                    // for DOT procs
                    if (!IsPositiveSpell(procSpell->Id))
                        return SPELL_AURA_PROC_FAILED;
                    break;
                }
                case 67702:                                 // Death's Choice, Item - Coliseum 25 Normal Melee Trinket
                {
                    float stat = 0.0f;
                    // strength
                    if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 67708;stat = GetStat(STAT_STRENGTH); }
                    // agility
                    if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 67703;                               }
                    break;
                }
                case 67771:                                 // Death's Choice (heroic), Item - Coliseum 25 Heroic Melee Trinket
                {
                    float stat = 0.0f;
                    // strength
                    if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 67773;stat = GetStat(STAT_STRENGTH); }
                    // agility
                    if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 67772;                               }
                    break;
                }
            }
            break;
        case SPELLFAMILY_MAGE:
            if (auraSpellInfo->SpellIconID == 2127)         // Blazing Speed
            {
                switch (auraSpellInfo->Id)
                {
                    case 31641:  // Rank 1
                    case 31642:  // Rank 2
                        trigger_spell_id = 31643;
                        break;
                    default:
                        sLog.outError("Unit::HandleProcTriggerSpellAuraProc: Spell %u miss possibly Blazing Speed",auraSpellInfo->Id);
                        return SPELL_AURA_PROC_FAILED;
                }
            }
            else if(auraSpellInfo->Id == 26467)             // Persistent Shield (Scarab Brooch trinket)
            {
                // This spell originally trigger 13567 - Dummy Trigger (vs dummy effect)
                basepoints[0] = damage * 15 / 100;
                target = pVictim;
                trigger_spell_id = 26470;
            }
            else if(auraSpellInfo->Id == 71761)             // Deep Freeze Immunity State
            {
                // spell applied only to permanent immunes to stun targets (bosses)
                if (pVictim->GetTypeId() != TYPEID_UNIT ||
                    (((Creature*)pVictim)->GetCreatureInfo()->MechanicImmuneMask & (1 << (MECHANIC_STUN - 1))) == 0)
                    return SPELL_AURA_PROC_FAILED;
            }
            break;
        case SPELLFAMILY_WARRIOR:
            // Deep Wounds (replace triggered spells to directly apply DoT), dot spell have finilyflags
            if (auraSpellInfo->SpellFamilyFlags == UI64LIT(0x0) && auraSpellInfo->SpellIconID == 243)
            {
                float weaponDamage;
                // DW should benefit of attack power, damage percent mods etc.
                // TODO: check if using offhand damage is correct and if it should be divided by 2
                if (haveOffhandWeapon() && getAttackTimer(BASE_ATTACK) > getAttackTimer(OFF_ATTACK))
                    weaponDamage = (GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE) + GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE))/2;
                else
                    weaponDamage = (GetFloatValue(UNIT_FIELD_MINDAMAGE) + GetFloatValue(UNIT_FIELD_MAXDAMAGE))/2;

                switch (auraSpellInfo->Id)
                {
                    case 12834: basepoints[0] = int32(weaponDamage * 16 / 100); break;
                    case 12849: basepoints[0] = int32(weaponDamage * 32 / 100); break;
                    case 12867: basepoints[0] = int32(weaponDamage * 48 / 100); break;
                    // Impossible case
                    default:
                        sLog.outError("Unit::HandleProcTriggerSpellAuraProc: DW unknown spell rank %u",auraSpellInfo->Id);
                        return SPELL_AURA_PROC_FAILED;
                }

                // 1 tick/sec * 6 sec = 6 ticks
                basepoints[0] /= 6;

                trigger_spell_id = 12721;
                break;
            }
            if (auraSpellInfo->Id == 50421)             // Scent of Blood
                trigger_spell_id = 50422;
            break;
        case SPELLFAMILY_WARLOCK:
        {
            // Drain Soul
            if (auraSpellInfo->SpellFamilyFlags & UI64LIT(0x0000000000004000))
            {
                // search for "Improved Drain Soul" dummy aura
                Unit::AuraList const& mDummyAura = GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = mDummyAura.begin(); i != mDummyAura.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK && (*i)->GetSpellProto()->SpellIconID == 113)
                    {
                        // basepoints of trigger spell stored in dummyeffect of spellProto
                        int32 basepoints = GetMaxPower(POWER_MANA) * (*i)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2) / 100;
                        CastCustomSpell(this, 18371, &basepoints, NULL, NULL, true, castItem, triggeredByAura);
                        break;
                    }
                }
                // Not remove charge (aura removed on death in any cases)
                // Need for correct work Drain Soul SPELL_AURA_CHANNEL_DEATH_ITEM aura
                return SPELL_AURA_PROC_FAILED;
            }
            // Nether Protection
            else if (auraSpellInfo->SpellIconID == 1985)
            {
                if (!procSpell)
                    return SPELL_AURA_PROC_FAILED;
                switch(GetFirstSchoolInMask(GetSpellSchoolMask(procSpell)))
                {
                    case SPELL_SCHOOL_NORMAL:
                        return SPELL_AURA_PROC_FAILED;                   // ignore
                    case SPELL_SCHOOL_HOLY:   trigger_spell_id = 54370; break;
                    case SPELL_SCHOOL_FIRE:   trigger_spell_id = 54371; break;
                    case SPELL_SCHOOL_NATURE: trigger_spell_id = 54375; break;
                    case SPELL_SCHOOL_FROST:  trigger_spell_id = 54372; break;
                    case SPELL_SCHOOL_SHADOW: trigger_spell_id = 54374; break;
                    case SPELL_SCHOOL_ARCANE: trigger_spell_id = 54373; break;
                    default:
                        return SPELL_AURA_PROC_FAILED;
                }
            }
            // Cheat Death
            else if (auraSpellInfo->Id == 28845)
            {
                // When your health drops below 20% ....
                if (GetHealth() - damage > GetMaxHealth() / 5 || GetHealth() < GetMaxHealth() / 5)
                    return SPELL_AURA_PROC_FAILED;
            }
            // Decimation
            else if (auraSpellInfo->Id == 63156 || auraSpellInfo->Id == 63158)
            {
                // Looking for dummy effect
                Aura *aur = GetAura(auraSpellInfo->Id, EFFECT_INDEX_1);
                if (!aur)
                    return SPELL_AURA_PROC_FAILED;

                // If target's health is not below equal certain value (35%) not proc
                if (int32(pVictim->GetHealth() * 100 / pVictim->GetMaxHealth()) > aur->GetModifier()->m_amount)
                    return SPELL_AURA_PROC_FAILED;
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Greater Heal Refund (Avatar Raiment set)
            if (auraSpellInfo->Id==37594)
            {
                // Not give if target already have full health
                if (pVictim->GetHealth() == pVictim->GetMaxHealth())
                    return SPELL_AURA_PROC_FAILED;
                // If your Greater Heal brings the target to full health, you gain $37595s1 mana.
                if (pVictim->GetHealth() + damage < pVictim->GetMaxHealth())
                    return SPELL_AURA_PROC_FAILED;
                trigger_spell_id = 37595;
            }
            // Blessed Recovery
            else if (auraSpellInfo->SpellIconID == 1875)
            {
                switch (auraSpellInfo->Id)
                {
                    case 27811: trigger_spell_id = 27813; break;
                    case 27815: trigger_spell_id = 27817; break;
                    case 27816: trigger_spell_id = 27818; break;
                    default:
                        sLog.outError("Unit::HandleProcTriggerSpellAuraProc: Spell %u not handled in BR", auraSpellInfo->Id);
                    return SPELL_AURA_PROC_FAILED;
                }
                basepoints[0] = damage * triggerAmount / 100 / 3;
                target = this;
            }
            // Glyph of Shadow Word: Pain
            else if (auraSpellInfo->Id == 55681)
                basepoints[0] = triggerAmount * GetCreateMana() / 100;
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Druid Forms Trinket
            if (auraSpellInfo->Id==37336)
            {
                switch(m_form)
                {
                    case FORM_NONE:     trigger_spell_id = 37344;break;
                    case FORM_CAT:      trigger_spell_id = 37341;break;
                    case FORM_BEAR:
                    case FORM_DIREBEAR: trigger_spell_id = 37340;break;
                    case FORM_TREE:     trigger_spell_id = 37342;break;
                    case FORM_MOONKIN:  trigger_spell_id = 37343;break;
                    default:
                        return SPELL_AURA_PROC_FAILED;
                }
            }
            // Druid T9 Feral Relic (Lacerate, Swipe, Mangle, and Shred)
            else if (auraSpellInfo->Id==67353)
            {
                switch(m_form)
                {
                    case FORM_CAT:      trigger_spell_id = 67355; break;
                    case FORM_BEAR:
                    case FORM_DIREBEAR: trigger_spell_id = 67354; break;
                    default:
                        return SPELL_AURA_PROC_FAILED;
                }
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
            // Piercing Shots
            if (auraSpellInfo->SpellIconID == 3247 && auraSpellInfo->SpellVisual[0] == 0)
            {
                basepoints[0] = damage * triggerAmount / 100 / 8;
                trigger_spell_id = 63468;
                target = pVictim;
            }
            // Rapid Recuperation
            else if (auraSpellInfo->Id == 53228 || auraSpellInfo->Id == 53232)
            {
                // This effect only from Rapid Fire (ability cast)
                if (!(procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000020)))
                    return SPELL_AURA_PROC_FAILED;
            }
            break;
        case SPELLFAMILY_PALADIN:
        {
            /*
            // Blessed Life
            if (auraSpellInfo->SpellIconID == 2137)
            {
                switch (auraSpellInfo->Id)
                {
                    case 31828:                         // Rank 1
                    case 31829:                         // Rank 2
                    case 31830:                         // Rank 3
                        break;
                    default:
                        sLog.outError("Unit::HandleProcTriggerSpellAuraProc: Spell %u miss posibly Blessed Life", auraSpellInfo->Id);
                        return SPELL_AURA_PROC_FAILED;
                }
            }
            */
            // Healing Discount
            if (auraSpellInfo->Id==37705)
            {
                trigger_spell_id = 37706;
                target = this;
            }
            // Soul Preserver
            if (auraSpellInfo->Id==60510)
            {
                trigger_spell_id = 60515;
                target = this;
            }
            // Illumination
            else if (auraSpellInfo->SpellIconID==241)
            {
                if(!procSpell)
                    return SPELL_AURA_PROC_FAILED;
                // procspell is triggered spell but we need mana cost of original casted spell
                uint32 originalSpellId = procSpell->Id;
                // Holy Shock heal
                if (procSpell->SpellFamilyFlags & UI64LIT(0x0001000000000000))
                {
                    switch(procSpell->Id)
                    {
                        case 25914: originalSpellId = 20473; break;
                        case 25913: originalSpellId = 20929; break;
                        case 25903: originalSpellId = 20930; break;
                        case 27175: originalSpellId = 27174; break;
                        case 33074: originalSpellId = 33072; break;
                        case 48820: originalSpellId = 48824; break;
                        case 48821: originalSpellId = 48825; break;
                        default:
                            sLog.outError("Unit::HandleProcTriggerSpellAuraProc: Spell %u not handled in HShock",procSpell->Id);
                           return SPELL_AURA_PROC_FAILED;
                    }
                }
                SpellEntry const *originalSpell = sSpellStore.LookupEntry(originalSpellId);
                if(!originalSpell)
                {
                    sLog.outError("Unit::HandleProcTriggerSpellAuraProc: Spell %u unknown but selected as original in Illu",originalSpellId);
                    return SPELL_AURA_PROC_FAILED;
                }
                // percent stored in effect 1 (class scripts) base points
                int32 cost = originalSpell->manaCost + originalSpell->ManaCostPercentage * GetCreateMana() / 100;
                basepoints[0] = cost*auraSpellInfo->CalculateSimpleValue(EFFECT_INDEX_1)/100;
                trigger_spell_id = 20272;
                target = this;
            }
            // Lightning Capacitor
            else if (auraSpellInfo->Id==37657)
            {
                if(!pVictim || !pVictim->isAlive())
                    return SPELL_AURA_PROC_FAILED;
                // stacking
                CastSpell(this, 37658, true, NULL, triggeredByAura);

                Aura * dummy = GetDummyAura(37658);
                // release at 3 aura in stack (cont contain in basepoint of trigger aura)
                if(!dummy || dummy->GetStackAmount() < triggerAmount)
                    return SPELL_AURA_PROC_FAILED;

                RemoveAurasDueToSpell(37658);
                trigger_spell_id = 37661;
                target = pVictim;
            }
            // Bonus Healing (Crystal Spire of Karabor mace)
            else if (auraSpellInfo->Id == 40971)
            {
                // If your target is below $s1% health
                if (pVictim->GetHealth() > pVictim->GetMaxHealth() * triggerAmount / 100)
                    return SPELL_AURA_PROC_FAILED;
            }
            // Thunder Capacitor
            else if (auraSpellInfo->Id == 54841)
            {
                if(!pVictim || !pVictim->isAlive())
                    return SPELL_AURA_PROC_FAILED;
                // stacking
                CastSpell(this, 54842, true, NULL, triggeredByAura);

                // counting
                Aura * dummy = GetDummyAura(54842);
                // release at 3 aura in stack (cont contain in basepoint of trigger aura)
                if(!dummy || dummy->GetStackAmount() < triggerAmount)
                    return SPELL_AURA_PROC_FAILED;

                RemoveAurasDueToSpell(54842);
                trigger_spell_id = 54843;
                target = pVictim;
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Lightning Shield (overwrite non existing triggered spell call in spell.dbc
            if (auraSpellInfo->SpellFamilyFlags & UI64LIT(0x0000000000000400))
            {
                switch(auraSpellInfo->Id)
                {
                    case 324:                           // Rank 1
                        trigger_spell_id = 26364; break;
                    case 325:                           // Rank 2
                        trigger_spell_id = 26365; break;
                    case 905:                           // Rank 3
                        trigger_spell_id = 26366; break;
                    case 945:                           // Rank 4
                        trigger_spell_id = 26367; break;
                    case 8134:                          // Rank 5
                        trigger_spell_id = 26369; break;
                    case 10431:                         // Rank 6
                        trigger_spell_id = 26370; break;
                    case 10432:                         // Rank 7
                        trigger_spell_id = 26363; break;
                    case 25469:                         // Rank 8
                        trigger_spell_id = 26371; break;
                    case 25472:                         // Rank 9
                        trigger_spell_id = 26372; break;
                    case 49280:                         // Rank 10
                        trigger_spell_id = 49278; break;
                    case 49281:                         // Rank 11
                        trigger_spell_id = 49279; break;
                    default:
                        sLog.outError("Unit::HandleProcTriggerSpellAuraProc: Spell %u not handled in LShield", auraSpellInfo->Id);
                    return SPELL_AURA_PROC_FAILED;
                }
            }
            // Lightning Shield (The Ten Storms set)
            else if (auraSpellInfo->Id == 23551)
            {
                trigger_spell_id = 23552;
                target = pVictim;
            }
            // Damage from Lightning Shield (The Ten Storms set)
            else if (auraSpellInfo->Id == 23552)
                trigger_spell_id = 27635;
            // Mana Surge (The Earthfury set)
            else if (auraSpellInfo->Id == 23572)
            {
                if(!procSpell)
                    return SPELL_AURA_PROC_FAILED;
                basepoints[0] = procSpell->manaCost * 35 / 100;
                trigger_spell_id = 23571;
                target = this;
            }
            // Nature's Guardian
            else if (auraSpellInfo->SpellIconID == 2013)
            {
                // Check health condition - should drop to less 30% (damage deal after this!)
                if (!(10*(int32(GetHealth() - damage)) < int32(3 * GetMaxHealth())))
                    return SPELL_AURA_PROC_FAILED;

                if(pVictim && pVictim->isAlive())
                    pVictim->getThreatManager().modifyThreatPercent(this,-10);

                basepoints[0] = triggerAmount * GetMaxHealth() / 100;
                trigger_spell_id = 31616;
                target = this;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Acclimation
            if (auraSpellInfo->SpellIconID == 1930)
            {
                if (!procSpell)
                    return SPELL_AURA_PROC_FAILED;
                switch(GetFirstSchoolInMask(GetSpellSchoolMask(procSpell)))
                {
                    case SPELL_SCHOOL_NORMAL:
                        return SPELL_AURA_PROC_FAILED;                   // ignore
                    case SPELL_SCHOOL_HOLY:   trigger_spell_id = 50490; break;
                    case SPELL_SCHOOL_FIRE:   trigger_spell_id = 50362; break;
                    case SPELL_SCHOOL_NATURE: trigger_spell_id = 50488; break;
                    case SPELL_SCHOOL_FROST:  trigger_spell_id = 50485; break;
                    case SPELL_SCHOOL_SHADOW: trigger_spell_id = 50489; break;
                    case SPELL_SCHOOL_ARCANE: trigger_spell_id = 50486; break;
                    default:
                        return SPELL_AURA_PROC_FAILED;
                }
            }
            // Blade Barrier
            else if (auraSpellInfo->SpellIconID == 85)
            {
                if (GetTypeId() != TYPEID_PLAYER || getClass() != CLASS_DEATH_KNIGHT ||
                    !((Player*)this)->IsBaseRuneSlotsOnCooldown(RUNE_BLOOD))
                    return SPELL_AURA_PROC_FAILED;
            }
            // Improved Blood Presence
            else if (auraSpellInfo->Id == 63611)
            {
                if (GetTypeId() != TYPEID_PLAYER || !((Player*)this)->isHonorOrXPTarget(pVictim) || !damage)
                    return SPELL_AURA_PROC_FAILED;
                basepoints[0] = triggerAmount * damage / 100;
                trigger_spell_id = 50475;
            }
            break;
        }
        default:
             break;
    }

    // All ok. Check current trigger spell
    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(trigger_spell_id);
    if (!triggerEntry)
    {
        // Not cast unknown spell
        // sLog.outError("Unit::HandleProcTriggerSpellAuraProc: Spell %u have 0 in EffectTriggered[%d], not handled custom case?",auraSpellInfo->Id,triggeredByAura->GetEffIndex());
        return SPELL_AURA_PROC_FAILED;
    }

    // not allow proc extra attack spell at extra attack
    if (m_extraAttacks && IsSpellHaveEffect(triggerEntry, SPELL_EFFECT_ADD_EXTRA_ATTACKS))
        return SPELL_AURA_PROC_FAILED;

    // Custom basepoints/target for exist spell
    // dummy basepoints or other customs
    switch(trigger_spell_id)
    {
        // Cast positive spell on enemy target
        case 7099:  // Curse of Mending
        case 39647: // Curse of Mending
        case 29494: // Temptation
        case 20233: // Improved Lay on Hands (cast on target)
        {
            target = pVictim;
            break;
        }
        // Combo points add triggers (need add combopoint only for main target, and after possible combopoints reset)
        case 15250: // Rogue Setup
        {
            if(!pVictim || pVictim != getVictim())   // applied only for main target
                return SPELL_AURA_PROC_FAILED;
            break;                                   // continue normal case
        }
        // Finish movies that add combo
        case 14189: // Seal Fate (Netherblade set)
        case 14157: // Ruthlessness
        {
            // Need add combopoint AFTER finish movie (or they dropped in finish phase)
            break;
        }
        // Bloodthirst (($m/100)% of max health)
        case 23880:
        {
            basepoints[0] = int32(GetMaxHealth() * triggerAmount / 100);
            break;
        }
        // Shamanistic Rage triggered spell
        case 30824:
        {
            basepoints[0] = int32(GetTotalAttackPowerValue(BASE_ATTACK) * triggerAmount / 100);
            break;
        }
        // Enlightenment (trigger only from mana cost spells)
        case 35095:
        {
            if(!procSpell || procSpell->powerType!=POWER_MANA || procSpell->manaCost==0 && procSpell->ManaCostPercentage==0 && procSpell->manaCostPerlevel==0)
                return SPELL_AURA_PROC_FAILED;
            break;
        }
        // Demonic Pact
        case 48090:
        {
            // As the spell is proced from pet's attack - find owner
            Unit* owner = GetOwner();
            if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                return SPELL_AURA_PROC_FAILED;

            // This spell doesn't stack, but refreshes duration. So we receive current bonuses to minus them later.
            int32 curBonus = 0;
            if (Aura* aur = owner->GetAura(48090, EFFECT_INDEX_0))
                curBonus = aur->GetModifier()->m_amount;
            int32 spellDamage  = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_MAGIC) - curBonus;
            if(spellDamage <= 0)
                return SPELL_AURA_PROC_FAILED;

            // percent stored in owner talent dummy
            AuraList const& dummyAuras = owner->GetAurasByType(SPELL_AURA_DUMMY);
            for (AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
            {
                if ((*i)->GetSpellProto()->SpellIconID == 3220)
                {
                    basepoints[0] = basepoints[1] = int32(spellDamage * (*i)->GetModifier()->m_amount / 100);
                    break;
                }
            }
            break;
        }
        // Sword and Board
        case 50227:
        {
            // Remove cooldown on Shield Slam
            if (GetTypeId() == TYPEID_PLAYER)
                ((Player*)this)->RemoveSpellCategoryCooldown(1209, true);
            break;
        }
        // Maelstrom Weapon
        case 53817:
        {
            // have rank dependent proc chance, ignore too often cases
            // PPM = 2.5 * (rank of talent),
            uint32 rank = sSpellMgr.GetSpellRank(auraSpellInfo->Id);
            // 5 rank -> 100% 4 rank -> 80% and etc from full rate
            if(!roll_chance_i(20*rank))
                return SPELL_AURA_PROC_FAILED;
            break;
        }
        // Brain Freeze
        case 57761:
        {
            if(!procSpell)
                return SPELL_AURA_PROC_FAILED;
            // For trigger from Blizzard need exist Improved Blizzard
            if (procSpell->SpellFamilyName==SPELLFAMILY_MAGE && (procSpell->SpellFamilyFlags & UI64LIT(0x0000000000000080)))
            {
                bool found = false;
                AuraList const& mOverrideClassScript = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for(AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
                {
                    int32 script = (*i)->GetModifier()->m_miscvalue;
                    if(script==836 || script==988 || script==989)
                    {
                        found=true;
                        break;
                    }
                }
                if(!found)
                    return SPELL_AURA_PROC_FAILED;
            }
            break;
        }
        // Astral Shift
        case 52179:
        {
            if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == pVictim)
                return SPELL_AURA_PROC_FAILED;

            // Need stun, fear or silence mechanic
            if (!(GetAllSpellMechanicMask(procSpell) & IMMUNE_TO_SILENCE_AND_STUN_AND_FEAR_MASK))
                return SPELL_AURA_PROC_FAILED;
            break;
        }
        // Burning Determination
        case 54748:
        {
            if(!procSpell)
                return SPELL_AURA_PROC_FAILED;
            // Need Interrupt or Silenced mechanic
            if (!(GetAllSpellMechanicMask(procSpell) & IMMUNE_TO_INTERRUPT_AND_SILENCE_MASK))
                return SPELL_AURA_PROC_FAILED;
            break;
        }
        // Lock and Load
        case 56453:
        {
            // Proc only from trap activation (from periodic proc another aura of this spell)
            if (!(procFlags & PROC_FLAG_ON_TRAP_ACTIVATION) || !roll_chance_i(triggerAmount))
                return SPELL_AURA_PROC_FAILED;
            break;
        }
        // Freezing Fog (Rime triggered)
        case 59052:
        {
            // Howling Blast cooldown reset
            if (GetTypeId() == TYPEID_PLAYER)
                ((Player*)this)->RemoveSpellCategoryCooldown(1248, true);
            break;
        }
        // Druid - Savage Defense
        case 62606:
        {
            basepoints[0] = int32(GetTotalAttackPowerValue(BASE_ATTACK) * triggerAmount / 100);
            break;
        }
    }

    if( cooldown && GetTypeId()==TYPEID_PLAYER && ((Player*)this)->HasSpellCooldown(trigger_spell_id))
        return SPELL_AURA_PROC_FAILED;

    // try detect target manually if not set
    if (target == NULL)
        target = !(procFlags & PROC_FLAG_SUCCESSFUL_POSITIVE_SPELL) && IsPositiveSpell(trigger_spell_id) ? this : pVictim;

    // default case
    if (!target || target!=this && !target->isAlive())
        return SPELL_AURA_PROC_FAILED;

    if (basepoints[EFFECT_INDEX_0] || basepoints[EFFECT_INDEX_1] || basepoints[EFFECT_INDEX_2])
        CastCustomSpell(target,trigger_spell_id,
            basepoints[EFFECT_INDEX_0] ? &basepoints[EFFECT_INDEX_0] : NULL,
            basepoints[EFFECT_INDEX_1] ? &basepoints[EFFECT_INDEX_1] : NULL,
            basepoints[EFFECT_INDEX_2] ? &basepoints[EFFECT_INDEX_2] : NULL,
            true, castItem, triggeredByAura);
    else
        CastSpell(target,trigger_spell_id,true,castItem,triggeredByAura);

    if( cooldown && GetTypeId()==TYPEID_PLAYER )
        ((Player*)this)->AddSpellCooldown(trigger_spell_id,0,time(NULL) + cooldown);

    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleProcTriggerDamageAuraProc(Unit *pVictim, uint32 damage, Aura* triggeredByAura, SpellEntry const *procSpell, uint32 procFlags, uint32 procEx, uint32 cooldown)
{
    SpellEntry const *spellInfo = triggeredByAura->GetSpellProto();
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "ProcDamageAndSpell: doing %u damage from spell id %u (triggered by auratype %u of spell %u)",
        triggeredByAura->GetModifier()->m_amount, spellInfo->Id, triggeredByAura->GetModifier()->m_auraname, triggeredByAura->GetId());
    SpellNonMeleeDamage damageInfo(this, pVictim, spellInfo->Id, SpellSchoolMask(spellInfo->SchoolMask));
    CalculateSpellDamage(&damageInfo, triggeredByAura->GetModifier()->m_amount, spellInfo);
    damageInfo.target->CalculateAbsorbResistBlock(this, &damageInfo, spellInfo);
    DealDamageMods(damageInfo.target,damageInfo.damage,&damageInfo.absorb);
    SendSpellNonMeleeDamageLog(&damageInfo);
    DealSpellDamage(&damageInfo, true);
    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleOverrideClassScriptAuraProc(Unit *pVictim, uint32 /*damage*/, Aura *triggeredByAura, SpellEntry const *procSpell, uint32 /*procFlag*/, uint32 /*procEx*/ ,uint32 cooldown)
{
    int32 scriptId = triggeredByAura->GetModifier()->m_miscvalue;

    if(!pVictim || !pVictim->isAlive())
        return SPELL_AURA_PROC_FAILED;

    Item* castItem = triggeredByAura->GetCastItemGUID() && GetTypeId()==TYPEID_PLAYER
        ? ((Player*)this)->GetItemByGuid(triggeredByAura->GetCastItemGUID()) : NULL;

    // Basepoints of trigger aura
    int32 triggerAmount = triggeredByAura->GetModifier()->m_amount;

    uint32 triggered_spell_id = 0;

    switch(scriptId)
    {
        case 836:                                           // Improved Blizzard (Rank 1)
        {
            if (!procSpell || procSpell->SpellVisual[0]!=9487)
                return SPELL_AURA_PROC_FAILED;
            triggered_spell_id = 12484;
            break;
        }
        case 988:                                           // Improved Blizzard (Rank 2)
        {
            if (!procSpell || procSpell->SpellVisual[0]!=9487)
                return SPELL_AURA_PROC_FAILED;
            triggered_spell_id = 12485;
            break;
        }
        case 989:                                           // Improved Blizzard (Rank 3)
        {
            if (!procSpell || procSpell->SpellVisual[0]!=9487)
                return SPELL_AURA_PROC_FAILED;
            triggered_spell_id = 12486;
            break;
        }
        case 4086:                                          // Improved Mend Pet (Rank 1)
        case 4087:                                          // Improved Mend Pet (Rank 2)
        {
            if(!roll_chance_i(triggerAmount))
                return SPELL_AURA_PROC_FAILED;

            triggered_spell_id = 24406;
            break;
        }
        case 4533:                                          // Dreamwalker Raiment 2 pieces bonus
        {
            // Chance 50%
            if (!roll_chance_i(50))
                return SPELL_AURA_PROC_FAILED;

            switch (pVictim->getPowerType())
            {
                case POWER_MANA:   triggered_spell_id = 28722; break;
                case POWER_RAGE:   triggered_spell_id = 28723; break;
                case POWER_ENERGY: triggered_spell_id = 28724; break;
                default:
                    return SPELL_AURA_PROC_FAILED;
            }
            break;
        }
        case 4537:                                          // Dreamwalker Raiment 6 pieces bonus
            triggered_spell_id = 28750;                     // Blessing of the Claw
            break;
        case 5497:                                          // Improved Mana Gems (Serpent-Coil Braid)
            triggered_spell_id = 37445;                     // Mana Surge
            break;
        case 6953:                                          // Warbringer
            RemoveAurasAtMechanicImmunity(IMMUNE_TO_ROOT_AND_SNARE_MASK,0,true);
            return SPELL_AURA_PROC_OK;
        case 7010:                                          // Revitalize (rank 1)
        case 7011:                                          // Revitalize (rank 2)
        case 7012:                                          // Revitalize (rank 3)
        {
            if(!roll_chance_i(triggerAmount))
                return SPELL_AURA_PROC_FAILED;

            switch( pVictim->getPowerType() )
            {
                case POWER_MANA:        triggered_spell_id = 48542; break;
                case POWER_RAGE:        triggered_spell_id = 48541; break;
                case POWER_ENERGY:      triggered_spell_id = 48540; break;
                case POWER_RUNIC_POWER: triggered_spell_id = 48543; break;
                default: return SPELL_AURA_PROC_FAILED;
            }
            break;
        }
    }

    // not processed
    if(!triggered_spell_id)
        return SPELL_AURA_PROC_FAILED;

    // standard non-dummy case
    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if(!triggerEntry)
    {
        sLog.outError("Unit::HandleOverrideClassScriptAuraProc: Spell %u triggering for class script id %u",triggered_spell_id,scriptId);
        return SPELL_AURA_PROC_FAILED;
    }

    if( cooldown && GetTypeId()==TYPEID_PLAYER && ((Player*)this)->HasSpellCooldown(triggered_spell_id))
        return SPELL_AURA_PROC_FAILED;

    CastSpell(pVictim, triggered_spell_id, true, castItem, triggeredByAura);

    if( cooldown && GetTypeId()==TYPEID_PLAYER )
        ((Player*)this)->AddSpellCooldown(triggered_spell_id,0,time(NULL) + cooldown);

    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleMendingAuraProc( Unit* /*pVictim*/, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/ )
{
    // aura can be deleted at casts
    SpellEntry const* spellProto = triggeredByAura->GetSpellProto();
    SpellEffectIndex effIdx = triggeredByAura->GetEffIndex();
    int32 heal = triggeredByAura->GetModifier()->m_amount;
    uint64 caster_guid = triggeredByAura->GetCasterGUID();

    // jumps
    int32 jumps = triggeredByAura->GetHolder()->GetAuraCharges()-1;

    // current aura expire
    triggeredByAura->GetHolder()->SetAuraCharges(1);             // will removed at next charges decrease

    // next target selection
    if(jumps > 0 && GetTypeId()==TYPEID_PLAYER && IS_PLAYER_GUID(caster_guid))
    {
        float radius;
        if (spellProto->EffectRadiusIndex[effIdx])
            radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellProto->EffectRadiusIndex[effIdx]));
        else
            radius = GetSpellMaxRange(sSpellRangeStore.LookupEntry(spellProto->rangeIndex));

        if(Player* caster = ((Player*)triggeredByAura->GetCaster()))
        {
            caster->ApplySpellMod(spellProto->Id, SPELLMOD_RADIUS, radius,NULL);

            if(Player* target = ((Player*)this)->GetNextRandomRaidMember(radius))
            {
                // aura will applied from caster, but spell casted from current aura holder
                SpellModifier *mod = new SpellModifier(SPELLMOD_CHARGES,SPELLMOD_FLAT,jumps-5,spellProto->Id,spellProto->SpellFamilyFlags,spellProto->SpellFamilyFlags2);

                // remove before apply next (locked against deleted)
                triggeredByAura->SetInUse(true);
                RemoveAurasByCasterSpell(spellProto->Id,caster->GetGUID());

                caster->AddSpellMod(mod, true);
                CastCustomSpell(target,spellProto->Id,&heal,NULL,NULL,true,NULL,triggeredByAura,caster->GetGUID());
                caster->AddSpellMod(mod, false);
                triggeredByAura->SetInUse(false);
            }
        }
    }

    // heal
    CastCustomSpell(this,33110,&heal,NULL,NULL,true,NULL,NULL,caster_guid);
    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleModCastingSpeedNotStackAuraProc(Unit* /*pVictim*/, uint32 /*damage*/, Aura* /*triggeredByAura*/, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/)
{
    // Skip melee hits or instant cast spells
    return !(procSpell == NULL || GetSpellCastTime(procSpell) == 0) ? SPELL_AURA_PROC_OK : SPELL_AURA_PROC_FAILED;
}

SpellAuraProcResult Unit::HandleReflectSpellsSchoolAuraProc(Unit* /*pVictim*/, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/)
{
    // Skip Melee hits and spells ws wrong school
    return !(procSpell == NULL || (triggeredByAura->GetModifier()->m_miscvalue & procSpell->SchoolMask) == 0) ? SPELL_AURA_PROC_OK : SPELL_AURA_PROC_FAILED;
}

SpellAuraProcResult Unit::HandleModPowerCostSchoolAuraProc(Unit* /*pVictim*/, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/)
{
    // Skip melee hits and spells ws wrong school or zero cost
    return !(procSpell == NULL ||
            (procSpell->manaCost == 0 && procSpell->ManaCostPercentage == 0) ||           // Cost check
            (triggeredByAura->GetModifier()->m_miscvalue & procSpell->SchoolMask) == 0) ? SPELL_AURA_PROC_OK : SPELL_AURA_PROC_FAILED;  // School check
}

SpellAuraProcResult Unit::HandleMechanicImmuneResistanceAuraProc(Unit* /*pVictim*/, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/)
{
    // Compare mechanic
   return !(procSpell==NULL || procSpell->Mechanic != triggeredByAura->GetModifier()->m_miscvalue)  ? SPELL_AURA_PROC_OK : SPELL_AURA_PROC_FAILED;
}

SpellAuraProcResult Unit::HandleModDamageFromCasterAuraProc(Unit* pVictim, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/)
{
    // Compare casters
    return triggeredByAura->GetCasterGUID() == pVictim->GetGUID() ? SPELL_AURA_PROC_OK : SPELL_AURA_PROC_FAILED;
}

SpellAuraProcResult Unit::HandleMaelstromWeaponAuraProc(Unit* /*pVictim*/, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 /*cooldown*/)
{
    // remove all stack;
    RemoveSpellsCausingAura(SPELL_AURA_MAELSTROM_WEAPON);
    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleAddPctModifierAuraProc(Unit* /*pVictim*/, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const *procSpell, uint32 /*procFlag*/, uint32 procEx, uint32 /*cooldown*/)
{
    SpellEntry const *spellInfo = triggeredByAura->GetSpellProto();
    Item* castItem = triggeredByAura->GetCastItemGUID() && GetTypeId()==TYPEID_PLAYER
    ? ((Player*)this)->GetItemByGuid(triggeredByAura->GetCastItemGUID()) : NULL;

    switch(spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_MAGE:
        {
            // Combustion
            if (spellInfo->Id == 11129)
            {
                //last charge and crit
                if (triggeredByAura->GetHolder()->GetAuraCharges() <= 1 && (procEx & PROC_EX_CRITICAL_HIT) )
                    return SPELL_AURA_PROC_OK;                        // charge counting (will removed)

                CastSpell(this, 28682, true, castItem, triggeredByAura);
                return (procEx & PROC_EX_CRITICAL_HIT) ? SPELL_AURA_PROC_OK : SPELL_AURA_PROC_FAILED; // charge update only at crit hits, no hidden cooldowns
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Glyph of Divinity
            if (spellInfo->Id == 11129)
            {
                // Lookup base amount mana restore
                for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
                {
                    if (procSpell->Effect[i] == SPELL_EFFECT_ENERGIZE)
                    {
                        int32 mana = procSpell->CalculateSimpleValue(SpellEffectIndex(i));
                        CastCustomSpell(this, 54986, NULL, &mana, NULL, true, castItem, triggeredByAura);
                        break;
                    }
                }
                return SPELL_AURA_PROC_OK;
            }
            break;
        }
    }
    return SPELL_AURA_PROC_OK;
}

SpellAuraProcResult Unit::HandleModDamagePercentDoneAuraProc(Unit* /*pVictim*/, uint32 /*damage*/, Aura* triggeredByAura, SpellEntry const *procSpell, uint32 /*procFlag*/, uint32 procEx, uint32 cooldown)
{
    SpellEntry const *spellInfo = triggeredByAura->GetSpellProto();
    Item* castItem = triggeredByAura->GetCastItemGUID() && GetTypeId()==TYPEID_PLAYER
    ? ((Player*)this)->GetItemByGuid(triggeredByAura->GetCastItemGUID()) : NULL;

    // Aspect of the Viper
    if (spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER && spellInfo->SpellFamilyFlags & UI64LIT(0x4000000000000))
    {
        uint32 maxmana = GetMaxPower(POWER_MANA);
        int32 bp = int32(maxmana* GetAttackTime(RANGED_ATTACK)/1000.0f/100.0f);

        if(cooldown && GetTypeId()==TYPEID_PLAYER && ((Player*)this)->HasSpellCooldown(34075))
            return SPELL_AURA_PROC_FAILED;

        CastCustomSpell(this, 34075, &bp, NULL, NULL, true, castItem, triggeredByAura);
    }
    // Arcane Blast
    else if (spellInfo->Id == 36032 && procSpell->SpellFamilyName == SPELLFAMILY_MAGE && procSpell->SpellIconID == 2294)
        // prevent proc from self(spell that triggered this aura)
        return SPELL_AURA_PROC_FAILED;

    return SPELL_AURA_PROC_OK;
}
