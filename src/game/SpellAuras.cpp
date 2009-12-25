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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/SingletonImp.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "BattleGround.h"
#include "CreatureAI.h"
#include "ScriptCalls.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Vehicle.h"
#include "CellImpl.h"

#define NULL_AURA_SLOT 0xFF

pAuraHandler AuraHandler[TOTAL_AURAS]=
{
    &Aura::HandleNULL,                                      //  0 SPELL_AURA_NONE
    &Aura::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
    &Aura::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
    &Aura::HandlePeriodicDamage,                            //  3 SPELL_AURA_PERIODIC_DAMAGE
    &Aura::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
    &Aura::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
    &Aura::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
    &Aura::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
    &Aura::HandlePeriodicHeal,                              //  8 SPELL_AURA_PERIODIC_HEAL
    &Aura::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
    &Aura::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
    &Aura::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
    &Aura::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
    &Aura::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &Aura::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN   implemented in Unit::MeleeDamageBonus and Unit::SpellBaseDamageBonusForVictim
    &Aura::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD      implemented in Unit::DealMeleeDamage
    &Aura::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
    &Aura::HandleNoImmediateEffect,                         // 17 SPELL_AURA_MOD_STEALTH_DETECT implemented in Unit::isVisibleForOrDetect
    &Aura::HandleInvisibility,                              // 18 SPELL_AURA_MOD_INVISIBILITY
    &Aura::HandleInvisibilityDetect,                        // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &Aura::HandleAuraModTotalHealthPercentRegen,            // 20 SPELL_AURA_OBS_MOD_HEALTH
    &Aura::HandleAuraModTotalManaPercentRegen,              // 21 SPELL_AURA_OBS_MOD_MANA
    &Aura::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
    &Aura::HandlePeriodicTriggerSpell,                      // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL
    &Aura::HandlePeriodicEnergize,                          // 24 SPELL_AURA_PERIODIC_ENERGIZE
    &Aura::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
    &Aura::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
    &Aura::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
    &Aura::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
    &Aura::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
    &Aura::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
    &Aura::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &Aura::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &Aura::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &Aura::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &Aura::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &Aura::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
    &Aura::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
    &Aura::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
    &Aura::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &Aura::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &Aura::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
    &Aura::HandleAuraProcTriggerSpell,                      // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
    &Aura::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
    &Aura::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
    &Aura::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
    &Aura::HandleUnused,                                    // 46 SPELL_AURA_46 (used in test spells 54054 and 54058, and spell 48050) (3.0.8a-3.2.2a)
    &Aura::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &Aura::HandleNULL,                                      // 48 SPELL_AURA_48 spell Napalm (area damage spell with additional delayed damage effect)
    &Aura::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &Aura::HandleNoImmediateEffect,                         // 50 SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT implemented in Unit::SpellCriticalHealingBonus
    &Aura::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &Aura::HandleAuraModCritPercent,                        // 52 SPELL_AURA_MOD_CRIT_PERCENT
    &Aura::HandlePeriodicLeech,                             // 53 SPELL_AURA_PERIODIC_LEECH
    &Aura::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
    &Aura::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &Aura::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
    &Aura::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &Aura::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &Aura::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &Aura::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &Aura::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &Aura::HandlePeriodicHealthFunnel,                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
    &Aura::HandleUnused,                                    // 63 unused (3.0.8a-3.2.2a) old SPELL_AURA_PERIODIC_MANA_FUNNEL
    &Aura::HandlePeriodicManaLeech,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH
    &Aura::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &Aura::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &Aura::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &Aura::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &Aura::HandleSchoolAbsorb,                              // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalcAbsorbResist
    &Aura::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS      Useless, used by only one spell 41560 that has only visual effect (3.2.2a)
    &Aura::HandleModSpellCritChanceShool,                   // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &Aura::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &Aura::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &Aura::HandleNoImmediateEffect,                         // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
    &Aura::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE           implemented in WorldSession::HandleMessagechatOpcode
    &Aura::HandleFarSight,                                  // 76 SPELL_AURA_FAR_SIGHT
    &Aura::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &Aura::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
    &Aura::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &Aura::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
    &Aura::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT       implemented in Unit::CalcAbsorbResist
    &Aura::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &Aura::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &Aura::HandleModRegen,                                  // 84 SPELL_AURA_MOD_REGEN
    &Aura::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN
    &Aura::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &Aura::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &Aura::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
    &Aura::HandlePeriodicDamagePCT,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &Aura::HandleUnused,                                    // 90 unused (3.0.8a-3.2.2a) old SPELL_AURA_MOD_RESIST_CHANCE
    &Aura::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &Aura::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &Aura::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &Aura::HandleNoImmediateEffect,                         // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::RegenerateAll
    &Aura::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &Aura::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
    &Aura::HandleManaShield,                                // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalcAbsorbResist
    &Aura::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &Aura::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &Aura::HandleUnused,                                    //100 SPELL_AURA_AURAS_VISIBLE obsolete 3.x? all player can see all auras now, but still have 2 spells including GM-spell (1852,2855)
    &Aura::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &Aura::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &Aura::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &Aura::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &Aura::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &Aura::HandleAddModifier,                               //107 SPELL_AURA_ADD_FLAT_MODIFIER
    &Aura::HandleAddModifier,                               //108 SPELL_AURA_ADD_PCT_MODIFIER
    &Aura::HandleAddTargetTrigger,                          //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &Aura::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
    &Aura::HandleNoImmediateEffect,                         //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER implemented in Unit::SelectMagnetTarget
    &Aura::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS implemented in diff functions.
    &Aura::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusForVictim
    &Aura::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT     imppemented in Player::RegenerateAll and Player::RegenerateHealth
    &Aura::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonus
    &Aura::HandleUnused,                                    //119 unused (3.0.8a-3.2.2a) old SPELL_AURA_SHARE_PET_TRACKING
    &Aura::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &Aura::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &Aura::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &Aura::HandleModTargetResistance,                       //123 SPELL_AURA_MOD_TARGET_RESISTANCE
    &Aura::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &Aura::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &Aura::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &Aura::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &Aura::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &Aura::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &Aura::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &Aura::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &Aura::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonus
    &Aura::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &Aura::HandleHaste,                                     //138 SPELL_AURA_MOD_HASTE
    &Aura::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
    &Aura::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
    &Aura::HandleRangedAmmoHaste,                           //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
    &Aura::HandleAuraModBaseResistancePCT,                  //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &Aura::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &Aura::HandleAuraSafeFall,                              //144 SPELL_AURA_SAFE_FALL                  implemented in WorldSession::HandleMovementOpcodes
    &Aura::HandleAuraModPetTalentsPoints,                   //145 SPELL_AURA_MOD_PET_TALENT_POINTS
    &Aura::HandleNoImmediateEffect,                         //146 SPELL_AURA_ALLOW_TAME_PET_TYPE        implemented in Player::CanTameExoticPets
    &Aura::HandleModMechanicImmunityMask,                   //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK     implemented in Unit::IsImmunedToSpell and Unit::IsImmunedToSpellEffect (check part)
    &Aura::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &Aura::HandleNoImmediateEffect,                         //149 SPELL_AURA_REDUCE_PUSHBACK            implemented in Spell::Delayed and Spell::DelayedChannel
    &Aura::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &Aura::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
    &Aura::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE         implemented in Creature::GetAttackDistance
    &Aura::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT          implemented in Unit::CalcAbsorbResist
    &Aura::HandleNoImmediateEffect,                         //154 SPELL_AURA_MOD_STEALTH_LEVEL          implemented in Unit::isVisibleForOrDetect
    &Aura::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING        implemented in Player::getMaxTimer
    &Aura::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN        implemented in Player::CalculateReputationGain
    &Aura::HandleUnused,                                    //157 SPELL_AURA_PET_DAMAGE_MULTI (single test like spell 20782, also single for 214 aura)
    &Aura::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &Aura::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT              implemented in Player::RewardHonor
    &Aura::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT implemented in Player::RegenerateAll and Player::RegenerateHealth
    &Aura::HandleAuraPowerBurn,                             //162 SPELL_AURA_POWER_BURN_MANA
    &Aura::HandleNoImmediateEffect,                         //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS_MELEE implememnted in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
    &Aura::HandleUnused,                                    //164 unused (3.0.8a-3.2.2a), only one test spell 10654
    &Aura::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &Aura::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &Aura::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonus, Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
    &Aura::HandleNULL,                                      //170 SPELL_AURA_DETECT_AMORE       different spells that ignore transformation effects
    &Aura::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &Aura::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &Aura::HandleUnused,                                    //173 unused (3.0.8a-3.2.2a) no spells, old SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &Aura::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonus
    &Aura::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonus
    &Aura::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &Aura::HandleNULL,                                      //177 SPELL_AURA_AOE_CHARM (22 spells)
    &Aura::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &Aura::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonus
    &Aura::HandleUnused,                                    //181 unused (3.0.8a-3.2.2a) old SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS
    &Aura::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &Aura::HandleNoImmediateEffect,                         //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746, implemented in ThreatCalcHelper::calcThreat
    &Aura::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
    &Aura::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
    &Aura::HandleModRating,                                 //189 SPELL_AURA_MOD_RATING
    &Aura::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
    &Aura::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
    &Aura::HandleModMeleeRangedSpeedPct,                    //192 SPELL_AURA_HASTE_MELEE
    &Aura::HandleModCombatSpeedPct,                         //193 SPELL_AURA_MELEE_SLOW (in fact combat (any type attack) speed pct)
    &Aura::HandleNoImmediateEffect,                         //194 SPELL_AURA_MOD_IGNORE_ABSORB_SCHOOL       implement in Unit::CalcNotIgnoreAbsorbDamage
    &Aura::HandleNoImmediateEffect,                         //195 SPELL_AURA_MOD_IGNORE_ABSORB_FOR_SPELL    implement in Unit::CalcNotIgnoreAbsorbDamage
    &Aura::HandleNULL,                                      //196 SPELL_AURA_MOD_COOLDOWN (single spell 24818 in 3.2.2a)
    &Aura::HandleNoImmediateEffect,                         //197 SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE implemented in Unit::SpellCriticalBonus Unit::GetUnitCriticalChance
    &Aura::HandleUnused,                                    //198 unused (3.0.8a-3.2.2a) old SPELL_AURA_MOD_ALL_WEAPON_SKILLS
    &Aura::HandleNoImmediateEffect,                         //199 SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT  implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //200 SPELL_AURA_MOD_KILL_XP_PCT                 implemented in Player::GiveXP
    &Aura::HandleAuraAllowFlight,                           //201 SPELL_AURA_FLY                             this aura enable flight mode...
    &Aura::HandleNoImmediateEffect,                         //202 SPELL_AURA_CANNOT_BE_DODGED                implemented in Unit::RollPhysicalOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //203 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE  implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
    &Aura::HandleNoImmediateEffect,                         //204 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
    &Aura::HandleNoImmediateEffect,                         //205 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_DAMAGE  implemented in Unit::SpellCriticalDamageBonus
    &Aura::HandleNULL,                                      //206 SPELL_AURA_MOD_SPEED_MOUNTED
    &Aura::HandleAuraModIncreaseFlightSpeed,                //207 SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED
    &Aura::HandleAuraModIncreaseFlightSpeed,                //208 SPELL_AURA_MOD_SPEED_FLIGHT, used only in spell: Flight Form (Passive)
    &Aura::HandleAuraModIncreaseFlightSpeed,                //209 SPELL_AURA_MOD_FLIGHT_SPEED_ALWAYS
    &Aura::HandleNULL,                                      //210 "Increase flight speed by"
    &Aura::HandleAuraModIncreaseFlightSpeed,                //211 SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK
    &Aura::HandleAuraModRangedAttackPowerOfStatPercent,     //212 SPELL_AURA_MOD_RANGED_ATTACK_POWER_OF_STAT_PERCENT
    &Aura::HandleNoImmediateEffect,                         //213 SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT implemented in Player::RewardRage
    &Aura::HandleUnused,                                    //214 Tamed Pet Passive (single test like spell 20782, also single for 157 aura)
    &Aura::HandleArenaPreparation,                          //215 SPELL_AURA_ARENA_PREPARATION
    &Aura::HandleModCastingSpeed,                           //216 SPELL_AURA_HASTE_SPELLS
    &Aura::HandleUnused,                                    //217 unused (3.0.8a-3.2.2a)
    &Aura::HandleAuraModRangedHaste,                        //218 SPELL_AURA_HASTE_RANGED
    &Aura::HandleModManaRegen,                              //219 SPELL_AURA_MOD_MANA_REGEN_FROM_STAT
    &Aura::HandleModRatingFromStat,                         //220 SPELL_AURA_MOD_RATING_FROM_STAT
    &Aura::HandleNULL,                                      //221 ignored
    &Aura::HandleUnused,                                    //222 unused (3.0.8a-3.2.2a) only for spell 44586 that not used in real spell cast
    &Aura::HandleNULL,                                      //223 dummy code (cast damage spell to attacker) and another dymmy (jump to another nearby raid member)
    &Aura::HandleUnused,                                    //224 unused (3.0.8a-3.2.2a)
    &Aura::HandleNoImmediateEffect,                         //225 SPELL_AURA_PRAYER_OF_MENDING
    &Aura::HandleAuraPeriodicDummy,                         //226 SPELL_AURA_PERIODIC_DUMMY
    &Aura::HandlePeriodicTriggerSpellWithValue,             //227 SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE
    &Aura::HandleNoImmediateEffect,                         //228 SPELL_AURA_DETECT_STEALTH
    &Aura::HandleNoImmediateEffect,                         //229 SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE        implemented in Unit::SpellDamageBonus
    &Aura::HandleAuraModIncreaseMaxHealth,                  //230 Commanding Shout
    &Aura::HandleNoImmediateEffect,                         //231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
    &Aura::HandleNoImmediateEffect,                         //232 SPELL_AURA_MECHANIC_DURATION_MOD           implement in Unit::CalculateSpellDuration
    &Aura::HandleNULL,                                      //233 set model id to the one of the creature with id m_modifier.m_miscvalue
    &Aura::HandleNoImmediateEffect,                         //234 SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK implement in Unit::CalculateSpellDuration
    &Aura::HandleAuraModDispelResist,                       //235 SPELL_AURA_MOD_DISPEL_RESIST               implement in Unit::MagicSpellHitResult
    &Aura::HandleAuraControlVehicle,                        //236 SPELL_AURA_CONTROL_VEHICLE
    &Aura::HandleModSpellDamagePercentFromAttackPower,      //237 SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER  implemented in Unit::SpellBaseDamageBonus
    &Aura::HandleModSpellHealingPercentFromAttackPower,     //238 SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER implemented in Unit::SpellBaseHealingBonus
    &Aura::HandleAuraModScale,                              //239 SPELL_AURA_MOD_SCALE_2 only in Noggenfogger Elixir (16595) before 2.3.0 aura 61
    &Aura::HandleAuraModExpertise,                          //240 SPELL_AURA_MOD_EXPERTISE
    &Aura::HandleForceMoveForward,                          //241 Forces the player to move forward
    &Aura::HandleUnused,                                    //242 SPELL_AURA_MOD_SPELL_DAMAGE_FROM_HEALING (only 2 test spels in 3.2.2a)
    &Aura::HandleNULL,                                      //243 faction reaction override spells
    &Aura::HandleComprehendLanguage,                        //244 SPELL_AURA_COMPREHEND_LANGUAGE
    &Aura::HandleNoImmediateEffect,                         //245 SPELL_AURA_MOD_DURATION_OF_MAGIC_EFFECTS     implemented in Unit::CalculateSpellDuration
    &Aura::HandleNoImmediateEffect,                         //246 SPELL_AURA_MOD_DURATION_OF_EFFECTS_BY_DISPEL implemented in Unit::CalculateSpellDuration
    &Aura::HandleNULL,                                      //247 target to become a clone of the caster
    &Aura::HandleNoImmediateEffect,                         //248 SPELL_AURA_MOD_COMBAT_RESULT_CHANCE         implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleAuraConvertRune,                           //249 SPELL_AURA_CONVERT_RUNE
    &Aura::HandleAuraModIncreaseHealth,                     //250 SPELL_AURA_MOD_INCREASE_HEALTH_2
    &Aura::HandleNULL,                                      //251 SPELL_AURA_MOD_ENEMY_DODGE
    &Aura::HandleNULL,                                      //252 haste all?
    &Aura::HandleNULL,                                      //253 SPELL_AURA_MOD_BLOCK_CRIT_CHANCE
    &Aura::HandleNULL,                                      //254 SPELL_AURA_MOD_DISARM_SHIELD disarm Shield
    &Aura::HandleNoImmediateEffect,                         //255 SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT    implemented in Unit::SpellDamageBonus
    &Aura::HandleNoReagentUseAura,                          //256 SPELL_AURA_NO_REAGENT_USE Use SpellClassMask for spell select
    &Aura::HandleNULL,                                      //257 SPELL_AURA_MOD_TARGET_RESIST_BY_SPELL_CLASS Use SpellClassMask for spell select
    &Aura::HandleNULL,                                      //258 SPELL_AURA_MOD_SPELL_VISUAL
    &Aura::HandleNULL,                                      //259 corrupt healing over time spell
    &Aura::HandleNoImmediateEffect,                         //260 SPELL_AURA_SCREEN_EFFECT (miscvalue = id in ScreenEffect.dbc) not required any code
    &Aura::HandlePhase,                                     //261 SPELL_AURA_PHASE undetectable invisibility?     implemented in Unit::isVisibleForOrDetect
    &Aura::HandleNULL,                                      //262 ignore combat/aura state?
    &Aura::HandleAllowOnlyAbility,                          //263 SPELL_AURA_ALLOW_ONLY_ABILITY player can use only abilities set in SpellClassMask
    &Aura::HandleUnused,                                    //264 unused (3.0.8a-3.2.2a)
    &Aura::HandleUnused,                                    //265 unused (3.0.8a-3.2.2a)
    &Aura::HandleUnused,                                    //266 unused (3.0.8a-3.2.2a)
    &Aura::HandleNoImmediateEffect,                         //267 SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL         implemented in Unit::IsImmunedToSpellEffect
    &Aura::HandleAuraModAttackPowerOfStatPercent,           //268 SPELL_AURA_MOD_ATTACK_POWER_OF_STAT_PERCENT
    &Aura::HandleNoImmediateEffect,                         //269 SPELL_AURA_MOD_IGNORE_DAMAGE_REDUCTION_SCHOOL   implemented in Unit::CalcNotIgnoreDamageRedunction
    &Aura::HandleUnused,                                    //270 SPELL_AURA_MOD_IGNORE_TARGET_RESIST (unused in 3.2.2a)
    &Aura::HandleNoImmediateEffect,                         //271 SPELL_AURA_MOD_DAMAGE_FROM_CASTER    implemented in Unit::SpellDamageBonus
    &Aura::HandleNoImmediateEffect,                         //272 SPELL_AURA_MAELSTROM_WEAPON (unclear use for aura, it used in (3.2.2a...3.3.0) in single spell 53817 that spellmode stacked and charged spell expected to be drop as stack
    &Aura::HandleNoImmediateEffect,                         //273 SPELL_AURA_X_RAY (client side implementation)
    &Aura::HandleNULL,                                      //274 proc free shot?
    &Aura::HandleNoImmediateEffect,                         //275 SPELL_AURA_MOD_IGNORE_SHAPESHIFT Use SpellClassMask for spell select
    &Aura::HandleNULL,                                      //276 mod damage % mechanic?
    &Aura::HandleNoImmediateEffect,                         //277 SPELL_AURA_MOD_MAX_AFFECTED_TARGETS Use SpellClassMask for spell select
    &Aura::HandleNULL,                                      //278 SPELL_AURA_MOD_DISARM_RANGED disarm ranged weapon
    &Aura::HandleNULL,                                      //279 visual effects? 58836 and 57507
    &Aura::HandleModTargetArmorPct,                         //280 SPELL_AURA_MOD_TARGET_ARMOR_PCT
    &Aura::HandleNoImmediateEffect,                         //281 SPELL_AURA_MOD_HONOR_GAIN             implemented in Player::RewardHonor
    &Aura::HandleAuraIncreaseBaseHealthPercent,             //282 SPELL_AURA_INCREASE_BASE_HEALTH_PERCENT
    &Aura::HandleNoImmediateEffect,                         //283 SPELL_AURA_MOD_HEALING_RECEIVED       implemented in Unit::SpellHealingBonus
    &Aura::HandleNULL,                                      //284 51 spells
    &Aura::HandleAuraModAttackPowerOfArmor,                 //285 SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR  implemented in Player::UpdateAttackPowerAndDamage
    &Aura::HandleNoImmediateEffect,                         //286 SPELL_AURA_ABILITY_PERIODIC_CRIT      implemented in Aura::IsCritFromAbilityAura called from Aura::PeriodicTick
    &Aura::HandleNoImmediateEffect,                         //287 SPELL_AURA_DEFLECT_SPELLS             implemented in Unit::MagicSpellHitResult and Unit::MeleeSpellHitResult
    &Aura::HandleNULL,                                      //288 increase parry/deflect, prevent attack (single spell used 67801)
    &Aura::HandleUnused,                                    //289 unused (3.2.2a)
    &Aura::HandleAuraModAllCritChance,                      //290 SPELL_AURA_MOD_ALL_CRIT_CHANCE
    &Aura::HandleNoImmediateEffect,                         //291 SPELL_AURA_MOD_QUEST_XP_PCT           implemented in Player::GiveXP
    &Aura::HandleNULL,                                      //292 call stabled pet
    &Aura::HandleNULL,                                      //293 3 spells
    &Aura::HandleNULL,                                      //294 2 spells, possible prevent mana regen
    &Aura::HandleUnused,                                    //295 unused (3.2.2a)
    &Aura::HandleNULL,                                      //296 2 spells
    &Aura::HandleNULL,                                      //297 1 spell (counter spell school?)
    &Aura::HandleUnused,                                    //298 unused (3.2.2a)
    &Aura::HandleUnused,                                    //299 unused (3.2.2a)
    &Aura::HandleNULL,                                      //300 3 spells (share damage?)
    &Aura::HandleNULL,                                      //301 5 spells
    &Aura::HandleUnused,                                    //302 unused (3.2.2a)
    &Aura::HandleNULL,                                      //303 17 spells
    &Aura::HandleNULL,                                      //304 2 spells (alcohol effect?)
    &Aura::HandleAuraModIncreaseSpeed,                      //305 SPELL_AURA_MOD_MINIMUM_SPEED
    &Aura::HandleNULL                                       //306 1 spell
};

static AuraType const frozenAuraTypes[] = { SPELL_AURA_MOD_ROOT, SPELL_AURA_MOD_STUN, SPELL_AURA_NONE };

Aura::Aura(SpellEntry const* spellproto, uint32 eff, int32 *currentBasePoints, Unit *target, Unit *caster, Item* castItem) :
m_spellmod(NULL), m_caster_guid(0), m_target(target), m_castItemGuid(castItem?castItem->GetGUID():0),
m_timeCla(1000), m_periodicTimer(0), m_removeMode(AURA_REMOVE_BY_DEFAULT), m_AuraDRGroup(DIMINISHING_NONE),
m_effIndex(eff), m_auraSlot(MAX_AURAS), m_auraFlags(AFLAG_NONE), m_auraLevel(1), m_procCharges(0), m_stackAmount(1),
m_positive(false), m_permanent(false), m_isPeriodic(false), m_isAreaAura(false), m_isPersistent(false),
m_isRemovedOnShapeLost(true), m_in_use(0), m_deleted(false)
{
    assert(target);

    assert(spellproto && spellproto == sSpellStore.LookupEntry( spellproto->Id ) && "`info` must be pointer to sSpellStore element");

    m_spellProto = spellproto;

    m_currentBasePoints = currentBasePoints ? *currentBasePoints : m_spellProto->EffectBasePoints[eff];

    m_isPassive = IsPassiveSpell(GetId());
    m_positive = IsPositiveEffect(GetId(), m_effIndex);

    m_isSingleTargetAura = IsSingleTargetSpell(m_spellProto);

    m_applyTime = time(NULL);

    int32 damage;
    if(!caster)
    {
        m_caster_guid = target->GetGUID();
        damage = m_currentBasePoints+1;                     // stored value-1
        m_maxduration = target->CalculateSpellDuration(m_spellProto, m_effIndex, target);
    }
    else
    {
        m_caster_guid = caster->GetGUID();

        damage        = caster->CalculateSpellDamage(m_spellProto,m_effIndex,m_currentBasePoints,target);
        m_maxduration = caster->CalculateSpellDuration(m_spellProto, m_effIndex, target);

        if (!damage && castItem && castItem->GetItemSuffixFactor())
        {
            ItemRandomSuffixEntry const *item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(castItem->GetItemRandomPropertyId()));
            if(item_rand_suffix)
            {
                for (int k = 0; k < 3; ++k)
                {
                    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(item_rand_suffix->enchant_id[k]);
                    if(pEnchant)
                    {
                        for (int t = 0; t < 3; ++t)
                            if(pEnchant->spellid[t] == m_spellProto->Id)
                        {
                            damage = uint32((item_rand_suffix->prefix[k]*castItem->GetItemSuffixFactor()) / 10000 );
                            break;
                        }
                    }

                    if(damage)
                        break;
                }
            }
        }
    }

    if(m_maxduration == -1 || m_isPassive && m_spellProto->DurationIndex == 0)
        m_permanent = true;

    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;

    if(!m_permanent && modOwner)
    {
        modOwner->ApplySpellMod(GetId(), SPELLMOD_DURATION, m_maxduration);
        // Get zero duration aura after - need set m_maxduration > 0 for apply/remove aura work
        if (m_maxduration<=0)
            m_maxduration = 1;
    }

    m_duration = m_maxduration;

    sLog.outDebug("Aura: construct Spellid : %u, Aura : %u Duration : %d Target : %d Damage : %d", m_spellProto->Id, m_spellProto->EffectApplyAuraName[eff], m_maxduration, m_spellProto->EffectImplicitTargetA[eff],damage);

    m_effIndex = eff;
    SetModifier(AuraType(m_spellProto->EffectApplyAuraName[eff]), damage, m_spellProto->EffectAmplitude[eff], m_spellProto->EffectMiscValue[eff]);

    // Apply periodic time mod
    if(modOwner && m_modifier.periodictime)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_ACTIVATION_TIME, m_modifier.periodictime);

    // Start periodic on next tick or at aura apply
    if (!(m_spellProto->AttributesEx5 & SPELL_ATTR_EX5_START_PERIODIC_AT_APPLY))
        m_periodicTimer += m_modifier.periodictime;

    m_isDeathPersist = IsDeathPersistentSpell(m_spellProto);

    m_procCharges = m_spellProto->procCharges;
    if(modOwner)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);

    m_isRemovedOnShapeLost = (m_caster_guid==m_target->GetGUID() &&
                              m_spellProto->Stances &&
                            !(m_spellProto->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) &&
                            !(m_spellProto->Attributes & SPELL_ATTR_NOT_SHAPESHIFT));

    if (caster && m_spellProto->Id == 22959)                // Improved Scorch
    {
        // Glyph of Improved Scorch
        if (Aura* glyph = caster->GetDummyAura(56371))
            m_stackAmount = glyph->GetModifier()->m_amount;
    }
}

Aura::~Aura()
{
}

AreaAura::AreaAura(SpellEntry const* spellproto, uint32 eff, int32 *currentBasePoints, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, target, caster, castItem)
{
    m_isAreaAura = true;

    // caster==NULL in constructor args if target==caster in fact
    Unit* caster_ptr = caster ? caster : target;

    m_radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(GetSpellProto()->EffectRadiusIndex[m_effIndex]));
    if(Player* modOwner = caster_ptr->GetSpellModOwner())
        modOwner->ApplySpellMod(GetId(), SPELLMOD_RADIUS, m_radius);

    switch(spellproto->Effect[eff])
    {
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
            m_areaAuraType = AREA_AURA_PARTY;
            if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->isTotem())
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
            m_areaAuraType = AREA_AURA_RAID;
            if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->isTotem())
                m_modifier.m_auraname = SPELL_AURA_NONE;
            // Light's Beacon not applied to caster itself (TODO: more generic check for another simialr spell if any?)
            else if (target == caster_ptr && m_spellProto->Id == 53651)
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
            m_areaAuraType = AREA_AURA_FRIEND;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
            m_areaAuraType = AREA_AURA_ENEMY;
            if (target == caster_ptr)
                m_modifier.m_auraname = SPELL_AURA_NONE;    // Do not do any effect on self
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
            m_areaAuraType = AREA_AURA_PET;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            m_areaAuraType = AREA_AURA_OWNER;
            if (target == caster_ptr)
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        default:
            sLog.outError("Wrong spell effect in AreaAura constructor");
            ASSERT(false);
            break;
    }
}

AreaAura::~AreaAura()
{
}

PersistentAreaAura::PersistentAreaAura(SpellEntry const* spellproto, uint32 eff, int32 *currentBasePoints, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, target, caster, castItem)
{
    m_isPersistent = true;
}

PersistentAreaAura::~PersistentAreaAura()
{
}

SingleEnemyTargetAura::SingleEnemyTargetAura(SpellEntry const* spellproto, uint32 eff, int32 *currentBasePoints, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, target, caster, castItem)
{
    if (caster)
        m_casters_target_guid = caster->GetTypeId()==TYPEID_PLAYER ? ((Player*)caster)->GetSelection() : caster->GetTargetGUID();
    else
        m_casters_target_guid = 0;
}

SingleEnemyTargetAura::~SingleEnemyTargetAura()
{
}

Unit* SingleEnemyTargetAura::GetTriggerTarget() const
{
    return ObjectAccessor::GetUnit(*m_target, m_casters_target_guid);
}

Aura* CreateAura(SpellEntry const* spellproto, uint32 eff, int32 *currentBasePoints, Unit *target, Unit *caster, Item* castItem)
{
    if (IsAreaAuraEffect(spellproto->Effect[eff]))
        return new AreaAura(spellproto, eff, currentBasePoints, target, caster, castItem);

    uint32 triggeredSpellId = spellproto->EffectTriggerSpell[eff];

    if(SpellEntry const* triggeredSpellInfo = sSpellStore.LookupEntry(triggeredSpellId))
        for (int i = 0; i < 3; ++i)
            if (triggeredSpellInfo->EffectImplicitTargetA[i] == TARGET_SINGLE_ENEMY)
                return new SingleEnemyTargetAura(spellproto, eff, currentBasePoints, target, caster, castItem);

    return new Aura(spellproto, eff, currentBasePoints, target, caster, castItem);
}

Unit* Aura::GetCaster() const
{
    if(m_caster_guid == m_target->GetGUID())
        return m_target;

    //return ObjectAccessor::GetUnit(*m_target,m_caster_guid);
    //must return caster even if it's in another grid/map
    Unit *unit = ObjectAccessor::GetUnitInWorld(*m_target,m_caster_guid);
    return unit && unit->IsInWorld() ? unit : NULL;
}

void Aura::SetModifier(AuraType t, int32 a, uint32 pt, int32 miscValue)
{
    m_modifier.m_auraname = t;
    m_modifier.m_amount = a;
    m_modifier.m_miscvalue = miscValue;
    m_modifier.periodictime = pt;
}

void Aura::Update(uint32 diff)
{
    if (m_duration > 0)
    {
        m_duration -= diff;
        if (m_duration < 0)
            m_duration = 0;
        m_timeCla -= diff;

        // GetEffIndex()==0 prevent double/triple apply manaPerSecond/manaPerSecondPerLevel to same spell with many auras
        // all spells with manaPerSecond/manaPerSecondPerLevel have aura in effect 0
        if(GetEffIndex()==0 && m_timeCla <= 0)
        {
            if(Unit* caster = GetCaster())
            {
                Powers powertype = Powers(m_spellProto->powerType);
                int32 manaPerSecond = m_spellProto->manaPerSecond + m_spellProto->manaPerSecondPerLevel * caster->getLevel();
                m_timeCla = 1*IN_MILISECONDS;
                if (manaPerSecond)
                {
                    if(powertype==POWER_HEALTH)
                        caster->ModifyHealth(-manaPerSecond);
                    else
                        caster->ModifyPower(powertype,-manaPerSecond);
                }
            }
        }
    }

    // Channeled aura required check distance from caster
    if(IsChanneledSpell(m_spellProto) && m_caster_guid != m_target->GetGUID())
    {
        Unit* caster = GetCaster();
        if(!caster)
        {
            m_target->RemoveAura(GetId(), GetEffIndex());
            return;
        }

        // need check distance for channeled target only
        if (caster->GetChannelObjectGUID() == m_target->GetGUID())
        {
            // Get spell range
            float max_range = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellProto->rangeIndex));

            if(Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_RANGE, max_range, NULL);

            if(!caster->IsWithinDistInMap(m_target, max_range))
            {
                m_target->RemoveAura(GetId(), GetEffIndex());
                return;
            }
        }
    }

    if(m_isPeriodic && (m_duration >= 0 || m_isPassive || m_permanent))
    {
        m_periodicTimer -= diff;
        if(m_periodicTimer <= 0) // tick also at m_periodicTimer==0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            // update before applying (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_modifier.periodictime;
            PeriodicTick();
        }
    }
}

void AreaAura::Update(uint32 diff)
{
    // update for the caster of the aura
    if(m_caster_guid == m_target->GetGUID())
    {
        Unit* caster = m_target;

        if( !caster->hasUnitState(UNIT_STAT_ISOLATED) )
        {
            Unit* owner = caster->GetCharmerOrOwner();
            if (!owner)
                owner = caster;
            std::list<Unit *> targets;

            switch(m_areaAuraType)
            {
                case AREA_AURA_PARTY:
                {
                    Group *pGroup = NULL;

                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        pGroup = ((Player*)owner)->GetGroup();

                    if( pGroup)
                    {
                        uint8 subgroup = ((Player*)owner)->GetSubGroup();
                        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                        {
                            Player* Target = itr->getSource();
                            if(Target && Target->isAlive() && Target->GetSubGroup()==subgroup && caster->IsFriendlyTo(Target))
                            {
                                if(caster->IsWithinDistInMap(Target, m_radius))
                                    targets.push_back(Target);
                                Pet *pet = Target->GetPet();
                                if(pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                    targets.push_back(pet);
                            }
                        }
                    }
                    else
                    {
                        // add owner
                        if( owner != caster && caster->IsWithinDistInMap(owner, m_radius) )
                            targets.push_back(owner);
                        // add caster's pet
                        Unit* pet = caster->GetPet();
                        if( pet && caster->IsWithinDistInMap(pet, m_radius))
                            targets.push_back(pet);
                    }
                    break;
                }
                case AREA_AURA_RAID:
                {
                    Group *pGroup = NULL;

                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        pGroup = ((Player*)owner)->GetGroup();

                    if( pGroup)
                    {
                        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                        {
                            Player* Target = itr->getSource();
                            if(Target && Target->isAlive() && caster->IsFriendlyTo(Target))
                            {
                                if(caster->IsWithinDistInMap(Target, m_radius))
                                    targets.push_back(Target);
                                Pet *pet = Target->GetPet();
                                if(pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                    targets.push_back(pet);
                            }
                        }
                    }
                    else
                    {
                        // add owner
                        if( owner != caster && caster->IsWithinDistInMap(owner, m_radius) )
                            targets.push_back(owner);
                        // add caster's pet
                        Unit* pet = caster->GetPet();
                        if( pet && caster->IsWithinDistInMap(pet, m_radius))
                            targets.push_back(pet);
                    }
                    break;
                }
                case AREA_AURA_FRIEND:
                {
                    CellPair p(MaNGOS::ComputeCellPair(caster->GetPositionX(), caster->GetPositionY()));
                    Cell cell(p);
                    cell.data.Part.reserved = ALL_DISTRICT;
                    cell.SetNoCreate();

                    MaNGOS::AnyFriendlyUnitInObjectRangeCheck u_check(caster, owner, m_radius);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck> searcher(caster,targets, u_check);
                    TypeContainerVisitor<MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck>, WorldTypeMapContainer > world_unit_searcher(searcher);
                    TypeContainerVisitor<MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck>, GridTypeMapContainer >  grid_unit_searcher(searcher);
                    CellLock<GridReadGuard> cell_lock(cell, p);
                    cell_lock->Visit(cell_lock, world_unit_searcher, *caster->GetMap(), *caster, m_radius);
                    cell_lock->Visit(cell_lock, grid_unit_searcher, *caster->GetMap(), *caster, m_radius);
                    break;
                }
                case AREA_AURA_ENEMY:
                {
                    CellPair p(MaNGOS::ComputeCellPair(caster->GetPositionX(), caster->GetPositionY()));
                    Cell cell(p);
                    cell.data.Part.reserved = ALL_DISTRICT;
                    cell.SetNoCreate();

                    MaNGOS::AnyAoETargetUnitInObjectRangeCheck u_check(caster, owner, m_radius); // No GetCharmer in searcher
                    MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck> searcher(caster, targets, u_check);
                    TypeContainerVisitor<MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck>, WorldTypeMapContainer > world_unit_searcher(searcher);
                    TypeContainerVisitor<MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck>, GridTypeMapContainer >  grid_unit_searcher(searcher);
                    CellLock<GridReadGuard> cell_lock(cell, p);
                    cell_lock->Visit(cell_lock, world_unit_searcher, *caster->GetMap(), *caster, m_radius);
                    cell_lock->Visit(cell_lock, grid_unit_searcher, *caster->GetMap(), *caster, m_radius);
                    break;
                }
                case AREA_AURA_OWNER:
                case AREA_AURA_PET:
                {
                    if(owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                        targets.push_back(owner);
                    break;
                }
            }

            for(std::list<Unit *>::iterator tIter = targets.begin(); tIter != targets.end(); tIter++)
            {
                // flag for seelction is need apply aura to current iteration target
                bool apply = true;

                // we need ignore present caster self applied are auras sometime
                // in cases if this only auras applied for spell effect
                Unit::spellEffectPair spair = Unit::spellEffectPair(GetId(), m_effIndex);
                for(Unit::AuraMap::const_iterator i = (*tIter)->GetAuras().lower_bound(spair); i != (*tIter)->GetAuras().upper_bound(spair); ++i)
                {
                    if (i->second->IsDeleted())
                        continue;

                    switch(m_areaAuraType)
                    {
                        case AREA_AURA_ENEMY:
                            // non caster self-casted auras (non stacked)
                            if(i->second->GetModifier()->m_auraname != SPELL_AURA_NONE)
                                apply = false;
                            break;
                        case AREA_AURA_RAID:
                            // non caster self-casted auras (stacked from diff. casters)
                            if(i->second->GetModifier()->m_auraname != SPELL_AURA_NONE  || i->second->GetCasterGUID() == GetCasterGUID())
                                apply = false;
                            break;
                        default:
                            // in generic case not allow stacking area auras
                            apply = false;
                            break;
                    }

                    if(!apply)
                        break;
                }

                if(!apply)
                    continue;

                if(SpellEntry const *actualSpellInfo = sSpellMgr.SelectAuraRankForPlayerLevel(GetSpellProto(), (*tIter)->getLevel()))
                {
                    int32 actualBasePoints = m_currentBasePoints;
                    // recalculate basepoints for lower rank (all AreaAura spell not use custom basepoints?)
                    if(actualSpellInfo != GetSpellProto())
                        actualBasePoints = actualSpellInfo->EffectBasePoints[m_effIndex];
                    AreaAura *aur = new AreaAura(actualSpellInfo, m_effIndex, &actualBasePoints, (*tIter), caster, NULL);
                    aur->SetAuraDuration(GetAuraDuration());
                    (*tIter)->AddAura(aur);
                }
            }
        }
        Aura::Update(diff);
    }
    else                                                    // aura at non-caster
    {
        Unit* caster = GetCaster();

        Aura::Update(diff);

        // remove aura if out-of-range from caster (after teleport for example)
        // or caster is isolated or caster no longer has the aura
        // or caster is (no longer) friendly
        bool needFriendly = (m_areaAuraType == AREA_AURA_ENEMY ? false : true);
        if( !caster || caster->hasUnitState(UNIT_STAT_ISOLATED) ||
            !caster->IsWithinDistInMap(m_target, m_radius)      ||
            !caster->HasAura(GetId(), GetEffIndex())            ||
            caster->IsFriendlyTo(m_target) != needFriendly
           )
        {
            m_target->RemoveAurasByCasterSpell(GetId(), GetEffIndex(),GetCasterGUID());
        }
        else if( m_areaAuraType == AREA_AURA_PARTY)         // check if in same sub group
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGUID() != m_target->GetGUID() && caster->GetGUID() != m_target->GetCharmerOrOwnerGUID())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if( pGroup )
                {
                    Player* checkTarget = m_target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if(!checkTarget || !pGroup->SameSubGroup(check, checkTarget))
                        m_target->RemoveAura(GetId(), GetEffIndex());
                }
                else
                    m_target->RemoveAura(GetId(), GetEffIndex());
            }
        }
        else if( m_areaAuraType == AREA_AURA_RAID)          // TODO: fix me!
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGUID() != m_target->GetGUID() && caster->GetGUID() != m_target->GetCharmerOrOwnerGUID())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if( pGroup )
                {
                    Player* checkTarget = m_target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if(!checkTarget)
                        m_target->RemoveAurasByCasterSpell(GetId(), GetEffIndex(), GetCasterGUID());
                }
                else
                    m_target->RemoveAurasByCasterSpell(GetId(), GetEffIndex(), GetCasterGUID());
            }
        }
        else if( m_areaAuraType == AREA_AURA_PET || m_areaAuraType == AREA_AURA_OWNER )
        {
            if( m_target->GetGUID() != caster->GetCharmerOrOwnerGUID() )
                m_target->RemoveAurasByCasterSpell(GetId(), GetEffIndex(), GetCasterGUID());
        }
    }
}

void PersistentAreaAura::Update(uint32 diff)
{
    bool remove = false;

    // remove the aura if its caster or the dynamic object causing it was removed
    // or if the target moves too far from the dynamic object
    if(Unit *caster = GetCaster())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId(), GetEffIndex());
        if (dynObj)
        {
            if (!m_target->IsWithinDistInMap(dynObj, dynObj->GetRadius()))
                remove = true;
        }
        else
            remove = true;
    }
    else
        remove = true;

    Aura::Update(diff);

    if(remove)
        m_target->RemoveAura(GetId(), GetEffIndex());
}

void Aura::ApplyModifier(bool apply, bool Real)
{
    AuraType aura = m_modifier.m_auraname;

    SetInUse(true);
    if(aura < TOTAL_AURAS)
        (*this.*AuraHandler [aura])(apply, Real);
    SetInUse(false);
}

bool Aura::IsNeedVisibleSlot(Unit const* caster) const
{
    bool totemAura = caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem();

    // passive auras (except totem auras) do not get placed in the slots
    if (m_isPassive && !totemAura)
        return false;

    // generic not caster case
    if (m_target != caster)
        return true;

    // special area auras case at caster
    switch(m_spellProto->Effect[GetEffIndex()])
    {
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
            return false;
        case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
            // not sure is totemAura  need, just preserve old code results
            return totemAura || m_modifier.m_auraname != SPELL_AURA_NONE;
        default: break;
    }

    return true;
}

void Aura::_AddAura()
{
    if (!GetId())
        return;
    if(!m_target)
        return;

    // Second aura if some spell
    bool secondaura = false;
    // Try find slot for aura
    uint8 slot = NULL_AURA_SLOT;
    // Lookup for some spell auras (and get slot from it)
    for(uint8 i = 0; i < m_effIndex; ++i)
    {
        Unit::spellEffectPair spair = Unit::spellEffectPair(GetId(), i);
        for(Unit::AuraMap::const_iterator itr = m_target->GetAuras().lower_bound(spair); itr != m_target->GetAuras().upper_bound(spair); ++itr)
        {
            // allow use single slot only by auras from same caster
            if(itr->second->GetCasterGUID()==GetCasterGUID())
            {
                slot = itr->second->GetAuraSlot();
                secondaura = true;
                break;
            }
        }
        if (secondaura)
            break;
    }
    // Lookup free slot
    if (!secondaura && m_target->GetVisibleAurasCount() < MAX_AURAS)
    {
        Unit::VisibleAuraMap const *visibleAuras = m_target->GetVisibleAuras();
        for(uint8 i = 0; i < MAX_AURAS; ++i)
        {
            Unit::VisibleAuraMap::const_iterator itr = visibleAuras->find(i);
            if(itr == visibleAuras->end())
            {
                slot = i;
                // update for out of range group members (on 1 slot use)
                m_target->UpdateAuraForGroup(slot);
                break;
            }
        }
    }

    Unit* caster = GetCaster();

    // set infinity cooldown state for spells
    if(caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_spellProto->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
        {
            Item* castItem = m_castItemGuid ? ((Player*)caster)->GetItemByGuid(m_castItemGuid) : NULL;
            ((Player*)caster)->AddSpellAndCategoryCooldowns(m_spellProto,castItem ? castItem->GetEntry() : 0, NULL,true);
        }
    }

    if (IsNeedVisibleSlot(caster))
    {
        SetAuraSlot( slot );
        if(slot < MAX_AURAS)                        // slot found send data to client
        {
            SetAura(false);
            SetAuraFlags((1 << GetEffIndex()) | AFLAG_NOT_CASTER | ((GetAuraMaxDuration() > 0) ? AFLAG_DURATION : AFLAG_NONE) | (IsPositive() ? AFLAG_POSITIVE : AFLAG_NEGATIVE));
            SetAuraLevel(caster ? caster->getLevel() : sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL));
            SendAuraUpdate(false);
        }

        //*****************************************************
        // Update target aura state flag (at 1 aura apply)
        // TODO: Make it easer
        //*****************************************************
        if (!secondaura)
        {
            // Sitdown on apply aura req seated
            if (m_spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED && !m_target->IsSitState())
                m_target->SetStandState(UNIT_STAND_STATE_SIT);

            // register aura diminishing on apply
            if (getDiminishGroup() != DIMINISHING_NONE )
                m_target->ApplyDiminishingAura(getDiminishGroup(), true);

            // Update Seals information
            if (IsSealSpell(m_spellProto))
                m_target->ModifyAuraState(AURA_STATE_JUDGEMENT, true);

            // Conflagrate aura state on Immolate and Shadowflame
            if (m_spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK &&
                // Immolate
                ((m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000004)) ||
                // Shadowflame
                (m_spellProto->SpellFamilyFlags2 & 0x00000002)))
                m_target->ModifyAuraState(AURA_STATE_CONFLAGRATE, true);

            // Faerie Fire (druid versions)
            if (m_spellProto->SpellFamilyName == SPELLFAMILY_DRUID && (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000400)))
                m_target->ModifyAuraState(AURA_STATE_FAERIE_FIRE, true);

            // Victorious
            if (m_spellProto->SpellFamilyName == SPELLFAMILY_WARRIOR && (m_spellProto->SpellFamilyFlags & UI64LIT(0x0004000000000000)))
                m_target->ModifyAuraState(AURA_STATE_WARRIOR_VICTORY_RUSH, true);

            // Swiftmend state on Regrowth & Rejuvenation
            if (m_spellProto->SpellFamilyName == SPELLFAMILY_DRUID && (m_spellProto->SpellFamilyFlags & UI64LIT(0x50)))
                m_target->ModifyAuraState(AURA_STATE_SWIFTMEND, true);

            // Deadly poison aura state
            if(m_spellProto->SpellFamilyName == SPELLFAMILY_ROGUE && (m_spellProto->SpellFamilyFlags & UI64LIT(0x10000)))
                m_target->ModifyAuraState(AURA_STATE_DEADLY_POISON, true);

            // Enrage aura state
            if(m_spellProto->Dispel == DISPEL_ENRAGE)
                m_target->ModifyAuraState(AURA_STATE_ENRAGE, true);
        }
    }
}

bool Aura::_RemoveAura()
{
    // Remove all triggered by aura spells vs unlimited duration
    // except same aura replace case
    if(m_removeMode!=AURA_REMOVE_BY_STACK)
        CleanupTriggeredSpells();

    Unit* caster = GetCaster();

    if(caster && IsPersistent())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId(), GetEffIndex());
        if (dynObj)
            dynObj->RemoveAffected(m_target);
    }

    //passive auras do not get put in slots
    // Note: but totem can be not accessible for aura target in time remove (to far for find in grid)
    //if(m_isPassive && !(caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem()))
    //    return;

    uint8 slot = GetAuraSlot();

    if(slot >= MAX_AURAS)                                   // slot not set
        return false;

    if(m_target->GetVisibleAura(slot) == 0)
        return false;

    bool lastaura = true;

    // find other aura in same slot (current already removed from list)
    for(uint8 i = 0; i < 3; ++i)
    {
        Unit::spellEffectPair spair = Unit::spellEffectPair(GetId(), i);
        for(Unit::AuraMap::const_iterator itr = m_target->GetAuras().lower_bound(spair); itr != m_target->GetAuras().upper_bound(spair); ++itr)
        {
            if(itr->second->GetAuraSlot() == slot)
            {
                lastaura = false;
                break;
            }
        }
        if(!lastaura)
            break;
    }

    // only remove icon when the last aura of the spell is removed (current aura already removed from list)
    if (!lastaura)
        return false;

    // unregister aura diminishing (and store last time)
    if (getDiminishGroup() != DIMINISHING_NONE )
        m_target->ApplyDiminishingAura(getDiminishGroup(), false);

    SetAura(true);
    SetAuraFlags(AFLAG_NONE);
    SetAuraLevel(0);

    if (m_removeMode != AURA_REMOVE_BY_DELETE)
    {
        SendAuraUpdate(true);

        // update for out of range group members
        m_target->UpdateAuraForGroup(slot);

        //*****************************************************
        // Update target aura state flag (at last aura remove)
        //*****************************************************
        // Enrage aura state
        if(m_spellProto->Dispel == DISPEL_ENRAGE)
            m_target->ModifyAuraState(AURA_STATE_ENRAGE, false);

        uint32 removeState = 0;
        uint64 removeFamilyFlag = m_spellProto->SpellFamilyFlags;
        uint32 removeFamilyFlag2 = m_spellProto->SpellFamilyFlags2;
        switch(m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_PALADIN:
                if (IsSealSpell(m_spellProto))
                    removeState = AURA_STATE_JUDGEMENT;     // Update Seals information
                break;
            case SPELLFAMILY_WARLOCK:
                // Conflagrate aura state on Immolate and Shadowflame,
                if ((m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000004)) ||
                    (m_spellProto->SpellFamilyFlags2 & 0x00000002))
                {
                    removeFamilyFlag = UI64LIT(0x0000000000000004);
                    removeFamilyFlag2 = 0x00000002;
                    removeState = AURA_STATE_CONFLAGRATE;
                }
                break;
            case SPELLFAMILY_DRUID:
                if(m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000400))
                    removeState = AURA_STATE_FAERIE_FIRE;   // Faerie Fire (druid versions)
                else if(m_spellProto->SpellFamilyFlags & UI64LIT(0x50))
                {
                    removeFamilyFlag = 0x50;
                    removeState = AURA_STATE_SWIFTMEND;     // Swiftmend aura state
                }
                break;
            case SPELLFAMILY_WARRIOR:
                if(m_spellProto->SpellFamilyFlags & UI64LIT(0x0004000000000000))
                    removeState = AURA_STATE_WARRIOR_VICTORY_RUSH; // Victorious
                break;
            case SPELLFAMILY_ROGUE:
                if(m_spellProto->SpellFamilyFlags & UI64LIT(0x10000))
                    removeState = AURA_STATE_DEADLY_POISON; // Deadly poison aura state
                break;
            case SPELLFAMILY_HUNTER:
                if(m_spellProto->SpellFamilyFlags & UI64LIT(0x1000000000000000))
                    removeState = AURA_STATE_FAERIE_FIRE;   // Sting (hunter versions)
        }

        // Remove state (but need check other auras for it)
        if (removeState)
        {
            bool found = false;
            Unit::AuraMap& Auras = m_target->GetAuras();
            for(Unit::AuraMap::iterator i = Auras.begin(); i != Auras.end(); ++i)
            {
                SpellEntry const *auraSpellInfo = (*i).second->GetSpellProto();
                if(auraSpellInfo->SpellFamilyName  == m_spellProto->SpellFamilyName &&
                    (auraSpellInfo->SpellFamilyFlags & removeFamilyFlag || auraSpellInfo->SpellFamilyFlags2 & removeFamilyFlag2))
                {
                    found = true;
                    break;
                }
            }
            // this has been last aura
            if(!found)
                m_target->ModifyAuraState(AuraState(removeState), false);
        }

        // reset cooldown state for spells
        if(caster && caster->GetTypeId() == TYPEID_PLAYER)
        {
            if ( GetSpellProto()->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE )
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existed cases)
                ((Player*)caster)->SendCooldownEvent(GetSpellProto());
        }
    }

    return true;
}

void Aura::SendAuraUpdate(bool remove)
{
    WorldPacket data(SMSG_AURA_UPDATE);
    data.append(m_target->GetPackGUID());
    data << uint8(GetAuraSlot());
    data << uint32(remove ? 0 : GetId());

    if(remove)
    {
        m_target->SendMessageToSet(&data, true);
        return;
    }

    uint8 auraFlags = GetAuraFlags();
    data << uint8(auraFlags);
    data << uint8(GetAuraLevel());
    data << uint8(m_procCharges ? m_procCharges*m_stackAmount : m_stackAmount);

    if(!(auraFlags & AFLAG_NOT_CASTER))
    {
        data << uint8(0);                                   // pguid
    }

    if(auraFlags & AFLAG_DURATION)
    {
        data << uint32(GetAuraMaxDuration());
        data << uint32(GetAuraDuration());
    }

    m_target->SendMessageToSet(&data, true);
}

void Aura::SetStackAmount(uint8 stackAmount)
{
    Unit *target = GetTarget();
    Unit *caster = GetCaster();
    if (!target || !caster)
        return;

    bool refresh = stackAmount >= m_stackAmount;
    if (stackAmount != m_stackAmount)
    {
        m_stackAmount = stackAmount;
        int32 amount = m_stackAmount * caster->CalculateSpellDamage(m_spellProto, m_effIndex, m_currentBasePoints, target);
        // Reapply if amount change
        if (amount!=m_modifier.m_amount)
        {
            ApplyModifier(false, true);
            m_modifier.m_amount = amount;
            ApplyModifier(true, true);
        }
    }

    if (refresh)
        // Stack increased refresh duration
        RefreshAura();
    else
        // Stack decreased only send update
        SendAuraUpdate(false);
}

bool Aura::modStackAmount(int32 num)
{
    // Can`t mod
    if (!m_spellProto->StackAmount)
        return true;

    // Modify stack but limit it
    int32 stackAmount = m_stackAmount + num;
    if (stackAmount > m_spellProto->StackAmount)
        stackAmount = m_spellProto->StackAmount;
    else if (stackAmount <=0) // Last aura from stack removed
    {
        m_stackAmount = 0;
        return true; // need remove aura
    }

    // Update stack amount
    SetStackAmount(stackAmount);
    return false;
}

void Aura::RefreshAura()
{
    m_duration = m_maxduration;
    SendAuraUpdate(false);
}

bool Aura::isAffectedOnSpell(SpellEntry const *spell) const
{
    // Check family name
    if (spell->SpellFamilyName != m_spellProto->SpellFamilyName)
        return false;
    // Check EffectClassMask
    uint32 const *ptr = getAuraSpellClassMask();
    if (((uint64*)ptr)[0] & spell->SpellFamilyFlags)
        return true;
    if (ptr[2] & spell->SpellFamilyFlags2)
        return true;
    return false;
}

void Aura::ReapplyAffectedPassiveAuras( Unit* target, bool owner_mode )
{
    std::set<uint32> affectedSelf;
    std::set<uint32> affectedAuraCaster;

    for(Unit::AuraMap::const_iterator itr = target->GetAuras().begin(); itr != target->GetAuras().end(); ++itr)
    {
        // permanent passive or permanent area aura
        // passive spells can be affected only by own or owner spell mods)
        if (itr->second->IsPermanent() && (owner_mode && itr->second->IsPassive() || itr->second->IsAreaAura()) &&
            // non deleted and not same aura (any with same spell id)
            !itr->second->IsDeleted() && itr->second->GetId() != GetId() &&
            // and affected by aura
            isAffectedOnSpell(itr->second->GetSpellProto()))
        {
            // only applied by self or aura caster
            if (itr->second->GetCasterGUID() == target->GetGUID())
                affectedSelf.insert(itr->second->GetId());
            else if (itr->second->GetCasterGUID() == GetCasterGUID())
                affectedAuraCaster.insert(itr->second->GetId());
        }
    }

    for(std::set<uint32>::const_iterator set_itr = affectedSelf.begin(); set_itr != affectedSelf.end(); ++set_itr)
    {
        target->RemoveAurasDueToSpell(*set_itr);
        target->CastSpell(m_target, *set_itr, true);
    }

    if (!affectedAuraCaster.empty())
    {
        Unit* caster = GetCaster();
        for(std::set<uint32>::const_iterator set_itr = affectedAuraCaster.begin(); set_itr != affectedAuraCaster.end(); ++set_itr)
        {
            target->RemoveAurasDueToSpell(*set_itr);
            if (caster)
                caster->CastSpell(m_target, *set_itr, true);
        }
    }
}

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER || !Real)
        return;

    if(m_modifier.m_miscvalue >= MAX_SPELLMOD)
        return;

    if (apply)
    {
        // Add custom charges for some mod aura
        switch (m_spellProto->Id)
        {
            case 17941:                                     // Shadow Trance
            case 22008:                                     // Netherwind Focus
            case 31834:                                     // Light's Grace
            case 34754:                                     // Clearcasting
            case 34936:                                     // Backlash
            case 44401:                                     // Missile Barrage 
            case 48108:                                     // Hot Streak
            case 51124:                                     // Killing Machine
            case 54741:                                     // Firestarter
            case 57761:                                     // Fireball!
            case 64823:                                     // Elune's Wrath (Balance druid t8 set
                SetAuraCharges(1);
                break;
        }

        m_spellmod = new SpellModifier(
            SpellModOp(m_modifier.m_miscvalue),
            SpellModType(m_modifier.m_auraname),            // SpellModType value == spell aura types
            m_modifier.m_amount,
            this,
            // prevent expire spell mods with (charges > 0 && m_stackAmount > 1)
            // all this spell expected expire not at use but at spell proc event check
            m_spellProto->StackAmount > 1 ? 0 : m_procCharges);
    }

    ((Player*)m_target)->AddSpellMod(m_spellmod, apply);

    // reapply talents to own passive persistent auras
    ReapplyAffectedPassiveAuras(m_target, true);

    // re-apply talents/passives/area auras applied to pet (it affected by player spellmods)
    if(Pet* pet = m_target->GetPet())
        ReapplyAffectedPassiveAuras(pet, true);

    // re-apply talents/passives/area auras applied to totems (it affected by player spellmods)
    for(int i = 0; i < MAX_TOTEM; ++i)
        if(m_target->m_TotemSlot[i])
            if(Creature* totem = m_target->GetMap()->GetCreature(m_target->m_TotemSlot[i]))
                ReapplyAffectedPassiveAuras(totem, true);

    // re-apply talents/passives/area auras applied to group members (it affected by player spellmods)
    if (Group* group = ((Player*)m_target)->GetGroup())
        for(GroupReference *itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            if (Player* member = itr->getSource())
                if (member != m_target && member->IsInMap(m_target))
                    ReapplyAffectedPassiveAuras(member, false);
}

void Aura::HandleAddTargetTrigger(bool apply, bool /*Real*/)
{
    // Use SpellModifier structure for check
    // used only fields:
    //  spellId, mask, mask2
    if (apply)
    {
        SpellModifier *mod = new SpellModifier;
        mod->spellId = GetId();

        uint32 const *ptr = m_spellProto->GetEffectSpellClassMask(m_effIndex);

        mod->mask = (uint64)ptr[0] | (uint64)ptr[1]<<32;
        mod->mask2= ptr[2];
        m_spellmod = mod;
    }
    else
    {
        delete m_spellmod;
        m_spellmod = NULL;
    }
}

void Aura::TriggerSpell()
{
    const uint64& casterGUID = GetCasterGUID();
    Unit* target = GetTriggerTarget();

    if(!casterGUID || !target)
        return;

    // generic casting code with custom spells and target/caster customs
    uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];

    SpellEntry const *triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
    SpellEntry const *auraSpellInfo = GetSpellProto();
    uint32 auraId = auraSpellInfo->Id;

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == NULL)
    {
        switch(auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch(auraId)
                {
                    // Firestone Passive (1-5 ranks)
                    case 758:
                    case 17945:
                    case 17947:
                    case 17949:
                    case 27252:
                    {
                        if (target->GetTypeId() != TYPEID_PLAYER)
                            return;
                        Item* item = ((Player*)target)->GetWeaponForAttack(BASE_ATTACK);
                        if (!item)
                            return;
                        uint32 enchant_id = 0;
                        switch (GetId())
                        {
                             case   758: enchant_id = 1803; break;   // Rank 1
                             case 17945: enchant_id = 1823; break;   // Rank 2
                             case 17947: enchant_id = 1824; break;   // Rank 3
                             case 17949: enchant_id = 1825; break;   // Rank 4
                             case 27252: enchant_id = 2645; break;   // Rank 5
                             default:
                                 return;
                        }
                        // remove old enchanting before applying new
                        ((Player*)target)->ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,false);
                        item->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, m_modifier.periodictime+1000, 0);
                        // add new enchanting
                        ((Player*)target)->ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,true);
                        return;
                    }
//                    // Periodic Mana Burn
//                    case 812: break;
//                    // Polymorphic Ray
//                    case 6965: break;
//                    // Fire Nova (1-7 ranks)
//                    case 8350:
//                    case 8508:
//                    case 8509:
//                    case 11312:
//                    case 11313:
//                    case 25540:
//                    case 25544:
//                        break;
                    // Thaumaturgy Channel
                    case 9712: trigger_spell_id = 21029; break;
//                    // Egan's Blaster
//                    case 17368: break;
//                    // Haunted
//                    case 18347: break;
//                    // Ranshalla Waiting
//                    case 18953: break;
//                    // Inferno
//                    case 19695: break;
//                    // Frostwolf Muzzle DND
//                    case 21794: break;
//                    // Alterac Ram Collar DND
//                    case 21866: break;
//                    // Celebras Waiting
//                    case 21916: break;
                    // Brood Affliction: Bronze
                    case 23170:
                    {
                        m_target->CastSpell(m_target, 23171, true, 0, this);
                        return;
                    }
//                    // Mark of Frost
//                    case 23184: break;
                    // Restoration
                    case 23493:
                    {
                        int32 heal = target->GetMaxHealth() / 10;
                        target->DealHeal(target, heal, auraSpellInfo);

                        if (int32 mana = target->GetMaxPower(POWER_MANA))
                        {
                            mana /= 10;
                            target->EnergizeBySpell(target, 23493, mana, POWER_MANA);
                        }
                        return;
                    }
//                    // Stoneclaw Totem Passive TEST
//                    case 23792: break;
//                    // Axe Flurry
//                    case 24018: break;
//                    // Mark of Arlokk
//                    case 24210: break;
//                    // Restoration
//                    case 24379: break;
//                    // Happy Pet
//                    case 24716: break;
//                    // Dream Fog
//                    case 24780: break;
//                    // Cannon Prep
//                    case 24832: break;
//                    // Shadow Bolt Whirl
//                    case 24834: break;
//                    // Stink Trap
//                    case 24918: break;
//                    // Mark of Nature
//                    case 25041: break;
//                    // Agro Drones
//                    case 25152: break;
                    // Consume
                    case 25371:
                    {
                        int32 bpDamage = target->GetMaxHealth()*10/100;
                        target->CastCustomSpell(target, 25373, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
                        return;
                    }
//                    // Pain Spike
//                    case 25572: break;
//                    // Rotate 360
//                    case 26009: break;
//                    // Rotate -360
//                    case 26136: break;
//                    // Consume
//                    case 26196: break;
//                    // Berserk
//                    case 26615: break;
//                    // Defile
//                    case 27177: break;
//                    // Teleport: IF/UC
//                    case 27601: break;
//                    // Five Fat Finger Exploding Heart Technique
//                    case 27673: break;
//                    // Nitrous Boost
//                    case 27746: break;
//                    // Steam Tank Passive
//                    case 27747: break;
                    // Frost Blast
                    case 27808:
                    {
                        int32 bpDamage = target->GetMaxHealth()*26/100;
                        target->CastCustomSpell(target, 29879, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
                        return;
                    }
//                    // Detonate Mana
//                    case 27819: break;
//                    // Controller Timer
//                    case 28095: break;
//                    // Stalagg Chain
//                    case 28096: break;
//                    // Stalagg Tesla Passive
//                    case 28097: break;
//                    // Feugen Tesla Passive
//                    case 28109: break;
//                    // Feugen Chain
//                    case 28111: break;
//                    // Mark of Didier
//                    case 28114: break;
//                    // Communique Timer, camp
//                    case 28346: break;
//                    // Icebolt
//                    case 28522: break;
//                    // Silithyst
//                    case 29519: break;
                    // Inoculate Nestlewood Owlkin
                    case 29528:
                        if(target->GetTypeId() != TYPEID_UNIT)// prevent error reports in case ignored player target
                            return;
                        break;
//                    // Overload
//                    case 29768: break;
//                    // Return Fire
//                    case 29788: break;
//                    // Return Fire
//                    case 29793: break;
//                    // Return Fire
//                    case 29794: break;
//                    // Guardian of Icecrown Passive
//                    case 29897: break;
                    // Feed Captured Animal
                    case 29917: trigger_spell_id = 29916; break;
//                    // Flame Wreath
//                    case 29946: break;
//                    // Flame Wreath
//                    case 29947: break;
//                    // Mind Exhaustion Passive
//                    case 30025: break;
//                    // Nether Beam - Serenity
//                    case 30401: break;
                    // Extract Gas
                    case 30427:
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                            return;
                        // move loot to player inventory and despawn target
                        if(caster->GetTypeId() ==TYPEID_PLAYER &&
                           target->GetTypeId() == TYPEID_UNIT &&
                           ((Creature*)target)->GetCreatureInfo()->type == CREATURE_TYPE_GAS_CLOUD)
                        {
                            Player* player = (Player*)caster;
                            Creature* creature = (Creature*)target;
                            // missing lootid has been reported on startup - just return
                            if (!creature->GetCreatureInfo()->SkinLootId)
                                return;

                            player->AutoStoreLoot(creature->GetCreatureInfo()->SkinLootId,LootTemplates_Skinning,true);

                            creature->ForcedDespawn();
                        }
                        return;
                    }
                    // Quake
                    case 30576: trigger_spell_id = 30571; break;
//                    // Burning Maul
//                    case 30598: break;
//                    // Regeneration
//                    case 30799:
//                    case 30800:
//                    case 30801:
//                        break;
//                    // Despawn Self - Smoke cloud
//                    case 31269: break;
//                    // Time Rift Periodic
//                    case 31320: break;
//                    // Corrupt Medivh
//                    case 31326: break;
                    // Doom
                    case 31347:
                    {
                        m_target->CastSpell(m_target,31350,true);
                        m_target->DealDamage(m_target, m_target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                        return;
                    }
                    // Spellcloth
                    case 31373:
                    {
                        // Summon Elemental after create item
                        target->SummonCreature(17870, 0, 0, 0, target->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                        return;
                    }
//                    // Bloodmyst Tesla
//                    case 31611: break;
                    // Doomfire
                    case 31944:
                    {
                        int32 damage = m_modifier.m_amount * ((float)(GetAuraDuration() + m_modifier.periodictime) / GetAuraMaxDuration());
                        target->CastCustomSpell(target, 31969, &damage, NULL, NULL, true, NULL, this, casterGUID);
                        return;
                    }
//                    // Teleport Test
//                    case 32236: break;
//                    // Earthquake
//                    case 32686: break;
//                    // Possess
//                    case 33401: break;
//                    // Draw Shadows
//                    case 33563: break;
//                    // Murmur's Touch
//                    case 33711: break;
                    // Flame Quills
                    case 34229:
                    {
                        // cast 24 spells 34269-34289, 34314-34316
                        for(uint32 spell_id = 34269; spell_id != 34290; ++spell_id)
                            target->CastSpell(target, spell_id, true, NULL, this, casterGUID);
                        for(uint32 spell_id = 34314; spell_id != 34317; ++spell_id)
                            target->CastSpell(target, spell_id, true, NULL, this, casterGUID);
                        return;
                    }
//                    // Gravity Lapse
//                    case 34480: break;
//                    // Tornado
//                    case 34683: break;
//                    // Frostbite Rotate
//                    case 34748: break;
//                    // Arcane Flurry
//                    case 34821: break;
//                    // Interrupt Shutdown
//                    case 35016: break;
//                    // Interrupt Shutdown
//                    case 35176: break;
//                    // Inferno
//                    case 35268: break;
//                    // Salaadin's Tesla
//                    case 35515: break;
//                    // Ethereal Channel (Red)
//                    case 35518: break;
//                    // Nether Vapor
//                    case 35879: break;
//                    // Dark Portal Storm
//                    case 36018: break;
//                    // Burning Maul
//                    case 36056: break;
//                    // Living Grove Defender Lifespan
//                    case 36061: break;
//                    // Professor Dabiri Talks
//                    case 36064: break;
//                    // Kael Gaining Power
//                    case 36091: break;
//                    // They Must Burn Bomb Aura
//                    case 36344: break;
//                    // They Must Burn Bomb Aura (self)
//                    case 36350: break;
//                    // Stolen Ravenous Ravager Egg
//                    case 36401: break;
//                    // Activated Cannon
//                    case 36410: break;
//                    // Stolen Ravenous Ravager Egg
//                    case 36418: break;
//                    // Enchanted Weapons
//                    case 36510: break;
//                    // Cursed Scarab Periodic
//                    case 36556: break;
//                    // Cursed Scarab Despawn Periodic
//                    case 36561: break;
//                    // Vision Guide
//                    case 36573: break;
//                    // Cannon Charging (platform)
//                    case 36785: break;
//                    // Cannon Charging (self)
//                    case 36860: break;
                    // Remote Toy
                    case 37027: trigger_spell_id = 37029; break;
//                    // Mark of Death
//                    case 37125: break;
//                    // Arcane Flurry
//                    case 37268: break;
//                    // Spout
//                    case 37429: break;
//                    // Spout
//                    case 37430: break;
//                    // Karazhan - Chess NPC AI, Snapshot timer
//                    case 37440: break;
//                    // Karazhan - Chess NPC AI, action timer
//                    case 37504: break;
//                    // Karazhan - Chess: Is Square OCCUPIED aura (DND)
//                    case 39400: break;
//                    // Banish
//                    case 37546: break;
//                    // Shriveling Gaze
//                    case 37589: break;
//                    // Fake Aggro Radius (2 yd)
//                    case 37815: break;
//                    // Corrupt Medivh
//                    case 37853: break;
                    // Eye of Grillok
                    case 38495:
                    {
                        m_target->CastSpell(m_target, 38530, true);
                        return;
                    }
                    // Absorb Eye of Grillok (Zezzak's Shard)
                    case 38554:
                    {
                        if(m_target->GetTypeId() != TYPEID_UNIT)
                            return;

                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 38495, true, NULL, this);
                        else
                            return;

                        Creature* creatureTarget = (Creature*)m_target;

                        creatureTarget->ForcedDespawn();
                        return;
                    }
//                    // Magic Sucker Device timer
//                    case 38672: break;
//                    // Tomb Guarding Charging
//                    case 38751: break;
//                    // Murmur's Touch
//                    case 38794: break;
//                    // Activate Nether-wraith Beacon (31742 Nether-wraith Beacon item)
//                    case 39105: break;
//                    // Drain World Tree Visual
//                    case 39140: break;
//                    // Quest - Dustin's Undead Dragon Visual aura
//                    case 39259: break;
//                    // Hellfire - The Exorcism, Jules releases darkness, aura
//                    case 39306: break;
//                    // Inferno
//                    case 39346: break;
//                    // Enchanted Weapons
//                    case 39489: break;
//                    // Shadow Bolt Whirl
//                    case 39630: break;
//                    // Shadow Bolt Whirl
//                    case 39634: break;
//                    // Shadow Inferno
//                    case 39645: break;
                    // Tear of Azzinoth Summon Channel - it's not really supposed to do anything,and this only prevents the console spam
                    case 39857: trigger_spell_id = 39856; break;
//                    // Soulgrinder Ritual Visual (Smashed)
//                    case 39974: break;
//                    // Simon Game Pre-game timer
//                    case 40041: break;
//                    // Knockdown Fel Cannon: The Aggro Check Aura
//                    case 40113: break;
//                    // Spirit Lance
//                    case 40157: break;
//                    // Demon Transform 2
//                    case 40398: break;
//                    // Demon Transform 1
//                    case 40511: break;
//                    // Ancient Flames
//                    case 40657: break;
//                    // Ethereal Ring Cannon: Cannon Aura
//                    case 40734: break;
//                    // Cage Trap
//                    case 40760: break;
//                    // Random Periodic
//                    case 40867: break;
//                    // Prismatic Shield
//                    case 40879: break;
//                    // Aura of Desire
//                    case 41350: break;
//                    // Dementia
//                    case 41404: break;
//                    // Chaos Form
//                    case 41629: break;
//                    // Alert Drums
//                    case 42177: break;
//                    // Spout
//                    case 42581: break;
//                    // Spout
//                    case 42582: break;
//                    // Return to the Spirit Realm
//                    case 44035: break;
//                    // Curse of Boundless Agony
//                    case 45050: break;
//                    // Earthquake
//                    case 46240: break;
                    // Personalized Weather
                    case 46736: trigger_spell_id = 46737; break;
//                    // Stay Submerged
//                    case 46981: break;
//                    // Dragonblight Ram
//                    case 47015: break;
//                    // Party G.R.E.N.A.D.E.
//                    case 51510: break;
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                switch(auraId)
                {
                    // Invisibility
                    case 66:
                    // Here need periodic triger reducing threat spell (or do it manually)
                        return;
                    default:
                        break;
                }
                break;
            }
//            case SPELLFAMILY_WARRIOR:
//            {
//                switch(auraId)
//                {
//                    // Wild Magic
//                    case 23410: break;
//                    // Corrupted Totems
//                    case 23425: break;
//                    default:
//                        break;
//                }
//                break;
//            }
//            case SPELLFAMILY_PRIEST:
//            {
//                switch(auraId)
//                {
//                    // Blue Beam
//                    case 32930: break;
//                    // Fury of the Dreghood Elders
//                    case 35460: break;
//                    default:
//                        break;
//                }
 //               break;
 //           }
            case SPELLFAMILY_HUNTER:
            {
                switch (auraId)
                {
                    // Sniper training
                    case 53302:
                    case 53303:
                    case 53304:
                        if (target->GetTypeId() != TYPEID_PLAYER)
                            return;

                        // Reset reapply counter at move
                        if (((Player*)target)->isMoving())
                        {
                            m_modifier.m_amount = 6;
                            return;
                        }

                        // We are standing at the moment
                        if (m_modifier.m_amount > 0)
                        {
                            --m_modifier.m_amount;
                            return;
                        }

                        // select rank of buff
                        switch(auraId)
                        {
                            case 53302: trigger_spell_id = 64418; break;
                            case 53303: trigger_spell_id = 64419; break;
                            case 53304: trigger_spell_id = 64420; break;
                        }

                        // If aura is active - no need to continue
                        if (target->HasAura(trigger_spell_id))
                            return;

                        break;
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                switch(auraId)
                {
                    // Cat Form
                    // trigger_spell_id not set and unknown effect triggered in this case, ignoring for while
                    case 768:
                        return;
                    // Frenzied Regeneration
                    case 22842:
                    case 22895:
                    case 22896:
                    case 26999:
                    {
                        int32 LifePerRage = GetModifier()->m_amount;

                        int32 lRage = m_target->GetPower(POWER_RAGE);
                        if(lRage > 100)                                     // rage stored as rage*10
                            lRage = 100;
                        m_target->ModifyPower(POWER_RAGE, -lRage);
                        int32 FRTriggerBasePoints = int32(lRage*LifePerRage/10);
                        m_target->CastCustomSpell(m_target, 22845, &FRTriggerBasePoints, NULL, NULL, true, NULL, this);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }

//            case SPELLFAMILY_HUNTER:
//            {
//                switch(auraId)
//                {
//                    //Frost Trap Aura
//                    case 13810:
//                        return;
//                    //Rizzle's Frost Trap
//                    case 39900:
//                        return;
//                    // Tame spells
//                    case 19597:         // Tame Ice Claw Bear
//                    case 19676:         // Tame Snow Leopard
//                    case 19677:         // Tame Large Crag Boar
//                    case 19678:         // Tame Adult Plainstrider
//                    case 19679:         // Tame Prairie Stalker
//                    case 19680:         // Tame Swoop
//                    case 19681:         // Tame Dire Mottled Boar
//                    case 19682:         // Tame Surf Crawler
//                    case 19683:         // Tame Armored Scorpid
//                    case 19684:         // Tame Webwood Lurker
//                    case 19685:         // Tame Nightsaber Stalker
//                    case 19686:         // Tame Strigid Screecher
//                    case 30100:         // Tame Crazed Dragonhawk
//                    case 30103:         // Tame Elder Springpaw
//                    case 30104:         // Tame Mistbat
//                    case 30647:         // Tame Barbed Crawler
//                    case 30648:         // Tame Greater Timberstrider
//                    case 30652:         // Tame Nightstalker
//                        return;
//                    default:
//                        break;
//                }
//                break;
//            }
            case SPELLFAMILY_SHAMAN:
            {
                switch(auraId)
                {
                    // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
                    case 28820:
                    {
                        // Need remove self if Lightning Shield not active
                        Unit::AuraMap const& auras = target->GetAuras();
                        for(Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            SpellEntry const* spell = itr->second->GetSpellProto();
                            if( spell->SpellFamilyName == SPELLFAMILY_SHAMAN &&
                                (spell->SpellFamilyFlags & UI64LIT(0x0000000000000400)))
                                return;
                        }
                        target->RemoveAurasDueToSpell(28820);
                        return;
                    }
                    // Totemic Mastery (Skyshatter Regalia (Shaman Tier 6) - bonus)
                    case 38443:
                    {
                        bool all = true;
                        for(int i = 0; i < MAX_TOTEM; ++i)
                        {
                            if(!target->m_TotemSlot[i])
                            {
                                all = false;
                                break;
                            }
                        }

                        if(all)
                            target->CastSpell(target, 38437, true, NULL, this);
                        else
                            target->RemoveAurasDueToSpell(38437);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }

        // Reget trigger spell proto
        triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
    }
    else
    {
        // Spell exist but require custom code
        switch(auraId)
        {
            // Curse of Idiocy
            case 1010:
            {
                // TODO: spell casted by result in correct way mostly
                // BUT:
                // 1) target show casting at each triggered cast: target don't must show casting animation for any triggered spell
                //      but must show affect apply like item casting
                // 2) maybe aura must be replace by new with accumulative stat mods instead stacking

                // prevent cast by triggered auras
                if(casterGUID == target->GetGUID())
                    return;

                // stop triggering after each affected stats lost > 90
                int32 intelectLoss = 0;
                int32 spiritLoss = 0;

                Unit::AuraList const& mModStat = target->GetAurasByType(SPELL_AURA_MOD_STAT);
                for(Unit::AuraList::const_iterator i = mModStat.begin(); i != mModStat.end(); ++i)
                {
                    if ((*i)->GetId() == 1010)
                    {
                        switch((*i)->GetModifier()->m_miscvalue)
                        {
                            case STAT_INTELLECT: intelectLoss += (*i)->GetModifier()->m_amount; break;
                            case STAT_SPIRIT:    spiritLoss   += (*i)->GetModifier()->m_amount; break;
                            default: break;
                        }
                    }
                }

                if(intelectLoss <= -90 && spiritLoss <= -90)
                    return;

                break;
            }
            // Mana Tide
            case 16191:
            {
                target->CastCustomSpell(target, trigger_spell_id, &m_modifier.m_amount, NULL, NULL, true, NULL, this);
                return;
            }
            // Ground Slam
            case 33525:
                target->CastSpell(target, trigger_spell_id, true, NULL, this, casterGUID);
                return;
            // Beacon of Light
            case 53563:
                // original caster must be target (beacon)
                target->CastSpell(target, trigger_spell_id, true, NULL, this, target->GetGUID());
                return;
            // Rapid Recuperation (triggered energize have baspioints == 0)
            case 56654:
            case 58882:
            {
                int32 mana = m_target->GetMaxPower(POWER_MANA) * m_modifier.m_amount / 100;
                target->CastCustomSpell(target, trigger_spell_id, &mana, NULL, NULL, true, NULL, this);
                return;
            }
        }
    }

    // All ok cast by default case
    if(triggeredSpellInfo)
        target->CastSpell(target, triggeredSpellInfo, true, NULL, this, casterGUID);
    else
    {
        if (Unit* caster = GetCaster())
        {
            if(target->GetTypeId() != TYPEID_UNIT || !Script->EffectDummyCreature(caster, GetId(), GetEffIndex(), (Creature*)target))
                sLog.outError("Aura::TriggerSpell: Spell %u have 0 in EffectTriggered[%d], not handled custom case?",GetId(),GetEffIndex());
        }
    }
}

void Aura::TriggerSpellWithValue()
{
    const uint64& casterGUID = GetCasterGUID();
    Unit* target = GetTriggerTarget();

    if(!casterGUID || !target)
        return;

    // generic casting code with custom spells and target/caster customs
    uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];
    int32  basepoints0 = this->GetModifier()->m_amount;

    target->CastCustomSpell(target, trigger_spell_id, &basepoints0, NULL, NULL, true, NULL, this, casterGUID);
}

/*********************************************************/
/***                  AURA EFFECTS                     ***/
/*********************************************************/

void Aura::HandleAuraDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // AT APPLY
    if(apply)
    {
        switch(m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch(GetId())
                {
                    case 1515:                              // Tame beast
                        // FIX_ME: this is 2.0.12 threat effect replaced in 2.1.x by dummy aura, must be checked for correctness
                        if (m_target->CanHaveThreatList())
                            if (Unit* caster = GetCaster())
                                m_target->AddThreat(caster, 10.0f, false, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
                        return;
                    case 7057:                              // Haunting Spirits
                        // expected to tick with 30 sec period (tick part see in Aura::PeriodicTick)
                        m_isPeriodic = true;
                        m_modifier.periodictime = 30*IN_MILISECONDS;
                        m_periodicTimer = m_modifier.periodictime;
                        return;
                    case 13139:                             // net-o-matic
                        // root to self part of (root_target->charge->root_self sequence
                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 13138, true, NULL, this);
                        return;
                    case 39850:                             // Rocket Blast
                        if(roll_chance_i(20))               // backfire stun
                            m_target->CastSpell(m_target, 51581, true, NULL, this);
                        return;
                    case 43873:                             // Headless Horseman Laugh
                        m_target->PlayDistanceSound(11965);
                        return;
                    case 46354:                             // Blood Elf Illusion
                        if (Unit* caster = GetCaster())
                        {
                            switch(caster->getGender())
                            {
                                case GENDER_FEMALE:
                                    caster->CastSpell(m_target, 46356, true, NULL, this);
                                    break;
                                case GENDER_MALE:
                                    caster->CastSpell(m_target, 46355, true, NULL, this);
                                    break;
                                default:
                                    break;
                            }
                        }
                        return;
                    case 46699:                             // Requires No Ammo
                        if (m_target->GetTypeId() == TYPEID_PLAYER)
                            // not use ammo and not allow use
                            ((Player*)m_target)->RemoveAmmo();
                        return;
                    case 62061:                             // Festive Holiday Mount
                        if (m_target->HasAuraType(SPELL_AURA_MOUNTED))
                            // Reindeer Transformation
                            m_target->CastSpell(m_target, 25860, true, NULL, this);
                        return;
                }
                break;
            case SPELLFAMILY_WARRIOR:
                // Overpower
                if(m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000004))
                {
                    // Must be casting target
                    if (!m_target->IsNonMeleeSpellCasted(false))
                        return;

                    Unit* caster = GetCaster();
                    if (!caster)
                        return;

                    Unit::AuraList const& modifierAuras = caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for(Unit::AuraList::const_iterator itr = modifierAuras.begin(); itr != modifierAuras.end(); ++itr)
                    {
                        // Unrelenting Assault
                        if((*itr)->GetSpellProto()->SpellFamilyName==SPELLFAMILY_WARRIOR && (*itr)->GetSpellProto()->SpellIconID == 2775)
                        {
                            switch ((*itr)->GetSpellProto()->Id)
                            {
                                case 46859:                 // Unrelenting Assault, rank 1
                                    m_target->CastSpell(m_target,64849,true,NULL,(*itr));
                                    break;
                                case 46860:                 // Unrelenting Assault, rank 2
                                    m_target->CastSpell(m_target,64850,true,NULL,(*itr));
                                    break;
                                default:
                                    break;
                            }
                            break;
                        }
                    }
                    return;
                }
                break;
            case SPELLFAMILY_SHAMAN:
                // Tidal Force
                if (GetId() == 55198)
                {
                    // apply max stack bufs
                    SpellEntry const* buffEntry = sSpellStore.LookupEntry(55166);
                    if (!buffEntry)
                        return;
                    for(int k = 0; k < buffEntry->StackAmount; ++k)
                        m_target->CastSpell(m_target, buffEntry, true, NULL, this);
                }
                // Earth Shield
                else if ((GetSpellProto()->SpellFamilyFlags & UI64LIT(0x40000000000)))
                {
                    // prevent double apply bonuses
                    if(m_target->GetTypeId() != TYPEID_PLAYER || !((Player*)m_target)->GetSession()->PlayerLoading())
                        if (Unit* caster = GetCaster())
                            m_modifier.m_amount = caster->SpellHealingBonus(m_target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                    return;
                }
                break;
        }
    }
    // AT REMOVE
    else
    {
        if( m_target->GetTypeId() == TYPEID_PLAYER &&
            ( GetSpellProto()->Effect[0]==72 || GetSpellProto()->Effect[0]==6 &&
            ( GetSpellProto()->EffectApplyAuraName[0]==1 || GetSpellProto()->EffectApplyAuraName[0]==128 ) ) )
        {
            // spells with SpellEffect=72 and aura=4: 6196, 6197, 21171, 21425
            ((Player*)m_target)->SetFarSightGUID(0);
            WorldPacket data(SMSG_CLEAR_FAR_SIGHT_IMMEDIATE, 0);
            ((Player*)m_target)->GetSession()->SendPacket(&data);
            return;
        }

        if (IsQuestTameSpell(GetId()) && m_target->isAlive())
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->isAlive())
                return;

            uint32 finalSpelId = 0;
            switch(GetId())
            {
                case 19548: finalSpelId = 19597; break;
                case 19674: finalSpelId = 19677; break;
                case 19687: finalSpelId = 19676; break;
                case 19688: finalSpelId = 19678; break;
                case 19689: finalSpelId = 19679; break;
                case 19692: finalSpelId = 19680; break;
                case 19693: finalSpelId = 19684; break;
                case 19694: finalSpelId = 19681; break;
                case 19696: finalSpelId = 19682; break;
                case 19697: finalSpelId = 19683; break;
                case 19699: finalSpelId = 19685; break;
                case 19700: finalSpelId = 19686; break;
                case 30646: finalSpelId = 30647; break;
                case 30653: finalSpelId = 30648; break;
                case 30654: finalSpelId = 30652; break;
                case 30099: finalSpelId = 30100; break;
                case 30102: finalSpelId = 30103; break;
                case 30105: finalSpelId = 30104; break;
            }

            if(finalSpelId)
                caster->CastSpell(m_target, finalSpelId, true, NULL, this);
            return;
        }

        switch(GetId())
        {
            case 28169:                                     // Mutating Injection
            {
                // Poison Cloud
                m_target->CastSpell(m_target, 28240, true, NULL, this);
                return;
            }
            case 36730:                                     // Flame Strike
            {
                m_target->CastSpell(m_target, 36731, true, NULL, this);
                return;
            }
            case 44191:                                     // Flame Strike
            {
                if (m_target->GetMap()->IsDungeon())
                {
                    uint32 spellId = m_target->GetMap()->IsRegularDifficulty() ? 44190 : 46163;

                    m_target->CastSpell(m_target, spellId, true, NULL, this);
                }
                return;
            }
            case 45934:                                     // Dark Fiend
            {
                // Kill target if dispelled
                if (m_removeMode==AURA_REMOVE_BY_DISPEL)
                    m_target->DealDamage(m_target, m_target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                return;
            }
            case 46308:                                     // Burning Winds
            {
                // casted only at creatures at spawn
                m_target->CastSpell(m_target, 47287, true, NULL, this);
                return;
            }
        }

        // Living Bomb
        if (m_spellProto->SpellFamilyName == SPELLFAMILY_MAGE && (m_spellProto->SpellFamilyFlags & UI64LIT(0x2000000000000)))
        {
            // Zero duration is equal to AURA_REMOVE_BY_DEFAULT. We can't use it directly, as it is set even
            // when removing aura from one target due to casting Living Bomb at other.
            if (m_duration == 0 || m_removeMode == AURA_REMOVE_BY_DISPEL)
                m_target->CastSpell(m_target,m_modifier.m_amount,true,NULL,this);
            return;
        }

        if (m_removeMode == AURA_REMOVE_BY_DEATH)
        {
            // Stop caster Arcane Missle chanelling on death
            if (m_spellProto->SpellFamilyName == SPELLFAMILY_MAGE &&
                (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000800)))
            {
                if (Unit* caster = GetCaster())
                    caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
                return;
            }
            // Stop caster Penance chanelling on death
            if (m_spellProto->SpellFamilyName == SPELLFAMILY_PRIEST &&
                (m_spellProto->SpellFamilyFlags2 & UI64LIT(0x00000080)))
            {
                if (Unit* caster = GetCaster())
                    caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
                return;
            }
        }
    }

    // AT APPLY & REMOVE

    switch(m_spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch(GetId())
            {
                // Recently Bandaged
                case 11196:
                    m_target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, GetMiscValue(), apply);
                    return;
                // Unstable Power
                case 24658:
                {
                    uint32 spellId = 24659;
                    if (apply)
                    {
                        SpellEntry const *spell = sSpellStore.LookupEntry(spellId);
                        Unit* caster = GetCaster();
                        if (!spell || !caster)
                            return;

                        for (int i=0; i < spell->StackAmount; ++i)
                            caster->CastSpell(m_target, spellId, true, NULL, NULL, GetCasterGUID());
                        return;
                    }
                    m_target->RemoveAurasDueToSpell(spellId);
                    return;
                }
                // Restless Strength
                case 24661:
                {
                    uint32 spellId = 24662;
                    if (apply)
                    {
                        SpellEntry const* spell = sSpellStore.LookupEntry(spellId);
                        Unit* caster = GetCaster();
                        if (!spell || !caster)
                            return;
                        for (int i=0; i < spell->StackAmount; ++i)
                            caster->CastSpell(m_target, spell->Id, true, NULL, NULL, GetCasterGUID());
                        return;
                    }
                    m_target->RemoveAurasDueToSpell(spellId);
                    return;
                }
                //Summon Fire Elemental
                case 40133:
                {
                    Unit* caster = GetCaster();
                    if (!caster)
                        return;

                    Unit *owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if(apply)
                            owner->CastSpell(owner, 8985, true);
                        else
                            ((Player*)owner)->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
                    }
                    return;
                }
                //Summon Earth Elemental
                case 40132 :
                {
                    Unit* caster = GetCaster();
                    if (!caster)
                        return;

                    Unit *owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if(apply)
                            owner->CastSpell(owner, 19704, true);
                        else
                            ((Player*)owner)->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
                    }
                    return;
                }
                //Dragonmaw Illusion
                case 40214 :
                {
                    if(apply)
                    {
                        m_target->CastSpell(m_target, 40216, true);
                        m_target->CastSpell(m_target, 42016, true);
                    }
                    else
                    {
                        m_target->RemoveAurasDueToSpell(40216);
                        m_target->RemoveAurasDueToSpell(42016);
                    }
                    return;
                }
                // LK Intro VO (1)
                case 58204:
                    if(m_target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 1
                        if(apply)
                            m_target->PlayDirectSound(14970, (Player *)m_target);
                        // continue in 58205
                        else
                            m_target->CastSpell(m_target, 58205, true);
                    }
                    return;
                // LK Intro VO (2)
                case 58205:
                    if(m_target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 2
                        if(apply)
                            m_target->PlayDirectSound(14971, (Player *)m_target);
                        // Play part 3
                        else
                            m_target->PlayDirectSound(14972, (Player *)m_target);
                    }
                    return;
                case 40131:
                case 27978:
                    if (apply)
                        m_target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    else
                        m_target->m_AuraFlags |= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    return;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
            break;
        case SPELLFAMILY_WARLOCK:
        {
            // Haunt
            if (GetSpellProto()->SpellIconID == 3172 && (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0004000000000000)))
            {
                // NOTE: for avoid use additional field damage stored in dummy value (replace unused 100%
                if (apply)
                    m_modifier.m_amount = 0;                // use value as damage counter instead redundent 100% percent
                else
                {
                    int32 bp0 = m_modifier.m_amount;

                    if (Unit* caster = GetCaster())
                        m_target->CastCustomSpell(caster,48210,&bp0,NULL,NULL,true);
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Pain and Suffering
            if( m_spellProto->SpellIconID == 2874 && m_target->GetTypeId()==TYPEID_PLAYER )
            {
                if(apply)
                {
                    // Reduce backfire damage (dot damage) from Shadow Word: Death
                    // aura have wrong effectclassmask, so use hardcoded value
                    m_spellmod = new SpellModifier(SPELLMOD_DOT,SPELLMOD_PCT,m_modifier.m_amount,GetId(),UI64LIT(0x0000200000000000));
                }
                ((Player*)m_target)->AddSpellMod(m_spellmod, apply);
                return;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch(GetId())
            {
                case 34246:                                 // Idol of the Emerald Queen
                case 60779:                                 // Idol of Lush Moss
                {
                    if (m_target->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if(apply)
                        // dummy not have proper effectclassmask
                        m_spellmod  = new SpellModifier(SPELLMOD_DOT,SPELLMOD_FLAT,m_modifier.m_amount/7,GetId(),UI64LIT(0x001000000000));

                    ((Player*)m_target)->AddSpellMod(m_spellmod, apply);
                    return;
                }
                case 52610:                                 // Savage Roar
                {
                    if(apply)
                    {
                        if(m_target->m_form != FORM_CAT)
                            return;

                        m_target->CastSpell(m_target, 62071, true);
                    }
                    else
                        m_target-> RemoveAurasDueToSpell(62071);
                    return;
                }
                case 61336:                                 // Survival Instincts
                {
                    if(apply)
                    {
                        if (!m_target->IsInFeralForm())
                            return;

                        int32 bp0 = int32(m_target->GetMaxHealth() * m_modifier.m_amount / 100);
                        m_target->CastCustomSpell(m_target, 50322, &bp0, NULL, NULL, true);
                    }
                    else
                        m_target-> RemoveAurasDueToSpell(50322);
                    return;
                }
            }

            // Lifebloom
            if (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x1000000000))
            {
                if ( apply )
                {
                    if (Unit* caster = GetCaster())
                        // prevent double apply bonuses
                        if(m_target->GetTypeId()!=TYPEID_PLAYER || !((Player*)m_target)->GetSession()->PlayerLoading())
                            m_modifier.m_amount = caster->SpellHealingBonus(m_target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                }
                else
                {
                    // Final heal only on dispelled or duration end
                    if ( !(GetAuraDuration() <= 0 || m_removeMode == AURA_REMOVE_BY_DISPEL) )
                        return;

                    // have a look if there is still some other Lifebloom dummy aura
                    Unit::AuraList const& auras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator itr = auras.begin(); itr!=auras.end(); ++itr)
                        if((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID &&
                            ((*itr)->GetSpellProto()->SpellFamilyFlags & UI64LIT(0x1000000000)))
                            return;

                    // final heal
                    if(m_target->IsInWorld() && m_stackAmount > 0)
                    {
                        int32 amount = m_modifier.m_amount / m_stackAmount;
                        m_target->CastCustomSpell(m_target, 33778, &amount, NULL, NULL, true, NULL, this, GetCasterGUID());

                        if (Unit* caster = GetCaster())
                        {
                            int32 returnmana = (GetSpellProto()->ManaCostPercentage * caster->GetCreateMana() / 100) * m_stackAmount / 2;
                            caster->CastCustomSpell(caster, 64372, &returnmana, NULL, NULL, true, NULL, this, GetCasterGUID());
                        }
                    }
                }
                return;
            }

            // Predatory Strikes
            if(m_target->GetTypeId()==TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
            {
                ((Player*)m_target)->UpdateAttackPowerAndDamage();
                return;
            }

            // Improved Moonkin Form
            if(GetSpellProto()->SpellIconID == 2855)
            {
                uint32 spell_id;
                switch(GetId())
                {
                    case 48384: spell_id = 50170; break;    //Rank 1
                    case 48395: spell_id = 50171; break;    //Rank 2
                    case 48396: spell_id = 50172; break;    //Rank 3
                    default:
                        sLog.outError("HandleAuraDummy: Not handled rank of IMF (Spell: %u)",GetId());
                        return;
                }

                if(apply)
                {
                    if(m_target->m_form != FORM_MOONKIN)
                        return;

                    m_target->CastSpell(m_target, spell_id, true);
                }
                else
                    m_target-> RemoveAurasDueToSpell(spell_id);
                return;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
            break;
        case SPELLFAMILY_PALADIN:
            switch(GetId())
            {
                case 20911:                                 // Blessing of Sanctuary
                case 25899:                                 // Greater Blessing of Sanctuary
                {
                    if (apply)
                        m_target->CastSpell(m_target, 67480, true, NULL, this);
                    else
                        m_target->RemoveAurasDueToSpell(67480);
                    return;
                }
            }
            break;
        case SPELLFAMILY_SHAMAN:
            break;
    }

    // pet auras
    if(PetAura const* petSpell = sSpellMgr.GetPetAura(GetId(), m_effIndex))
    {
        if(apply)
            m_target->AddPetAura(petSpell);
        else
            m_target->RemovePetAura(petSpell);
        return;
    }

    if(GetEffIndex()==0 && m_target->GetTypeId()==TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if(saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            m_target->GetZoneAndAreaId(zone, area);

            for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                // some auras remove at aura remove
                if(!itr->second->IsFitToRequirements((Player*)m_target, zone, area))
                    m_target->RemoveAurasDueToSpell(itr->second->spellId);
                // some auras applied at aura apply
                else if(itr->second->autocast)
                {
                    if( !m_target->HasAura(itr->second->spellId, 0) )
                        m_target->CastSpell(m_target, itr->second->spellId, true);
                }
            }
        }
    }
}

void Aura::HandleAuraMounted(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    if(apply)
    {
        CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
        if(!ci)
        {
            sLog.outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need it modelid)", m_modifier.m_miscvalue);
            return;
        }

        uint32 team = 0;
        if (m_target->GetTypeId()==TYPEID_PLAYER)
            team = ((Player*)m_target)->GetTeam();

        uint32 display_id = sObjectMgr.ChooseDisplayId(team,ci);
        CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
        if (minfo)
            display_id = minfo->modelid;

        m_target->Mount(display_id);
    }
    else
    {
        m_target->Unmount();
    }
}

void Aura::HandleAuraWaterWalk(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_WATER_WALK, 8+4);
    else
        data.Initialize(SMSG_MOVE_LAND_WALK, 8+4);
    data.append(m_target->GetPackGUID());
    data << uint32(0);
    m_target->SendMessageToSet(&data, true);
}

void Aura::HandleAuraFeatherFall(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_FEATHER_FALL, 8+4);
    else
        data.Initialize(SMSG_MOVE_NORMAL_FALL, 8+4);
    data.append(m_target->GetPackGUID());
    data << uint32(0);
    m_target->SendMessageToSet(&data, true);

    // start fall from current height
    if(!apply && m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->SetFallInformation(0, m_target->GetPositionZ());
}

void Aura::HandleAuraHover(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_SET_HOVER, 8+4);
    else
        data.Initialize(SMSG_MOVE_UNSET_HOVER, 8+4);
    data.append(m_target->GetPackGUID());
    data << uint32(0);
    m_target->SendMessageToSet(&data, true);
}

void Aura::HandleWaterBreathing(bool /*apply*/, bool /*Real*/)
{
    // update timers in client
    if(m_target->GetTypeId()==TYPEID_PLAYER)
        ((Player*)m_target)->UpdateMirrorTimers();
}

void Aura::HandleAuraModShapeshift(bool apply, bool Real)
{
    if(!Real)
        return;

    uint32 modelid = 0;
    Powers PowerType = POWER_MANA;
    ShapeshiftForm form = ShapeshiftForm(m_modifier.m_miscvalue);

    SpellShapeshiftEntry const* ssEntry = sSpellShapeshiftStore.LookupEntry(form);
    if (!ssEntry)
    {
        sLog.outError("Unknown shapeshift form %u in spell %u", form, GetId());
        return;
    }

    if (ssEntry->modelID_A)
    {
        // i will asume that creatures will always take the defined model from the dbc
        // since no field in creature_templates describes wether an alliance or
        // horde modelid should be used at shapeshifting
        if (m_target->GetTypeId() != TYPEID_PLAYER)
            modelid = ssEntry->modelID_A;
        else
        {
            // players are a bit difficult since the dbc has seldomly an horde modelid
            // so we add hacks here to set the right model
            if (Player::TeamForRace(m_target->getRace()) == ALLIANCE)
                modelid = ssEntry->modelID_A;
            else                                            // 3.2.3 only the moonkin form has this information
                modelid = ssEntry->modelID_H;

            // no model found, if player is horde we look here for our hardcoded modelids
            if (!modelid && Player::TeamForRace(m_target->getRace()) == HORDE)
            {

                switch(form)
                {
                    case FORM_CAT:
                        modelid = 8571;
                        break;
                    case FORM_BEAR:
                    case FORM_DIREBEAR:
                        modelid = 2289;
                        break;
                    case FORM_FLIGHT:
                        modelid = 20872;
                        break;
                    case FORM_FLIGHT_EPIC:
                        modelid = 21244;
                        break;
                    // per default use alliance modelid
                    // mostly horde and alliance share the same
                    default:
                        modelid = ssEntry->modelID_A;
                        break;
                }
            }
        }
    }

    // now only powertype must be set
    switch(form)
    {
        case FORM_CAT:
            PowerType = POWER_ENERGY;
            break;
        case FORM_BEAR:
        case FORM_DIREBEAR:
        case FORM_BATTLESTANCE:
        case FORM_BERSERKERSTANCE:
        case FORM_DEFENSIVESTANCE:
            PowerType = POWER_RAGE;
            break;
        default:
            break;
    }

    // remove polymorph before changing display id to keep new display id
    switch ( form )
    {
        case FORM_CAT:
        case FORM_TREE:
        case FORM_TRAVEL:
        case FORM_AQUA:
        case FORM_BEAR:
        case FORM_DIREBEAR:
        case FORM_FLIGHT_EPIC:
        case FORM_FLIGHT:
        case FORM_MOONKIN:
        {
            // remove movement affects
            m_target->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
            Unit::AuraList const& slowingAuras = m_target->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
            for (Unit::AuraList::const_iterator iter = slowingAuras.begin(); iter != slowingAuras.end();)
            {
                SpellEntry const* aurSpellInfo = (*iter)->GetSpellProto();

                uint32 aurMechMask = GetAllSpellMechanicMask(aurSpellInfo);

                // If spell that caused this aura has Croud Control or Daze effect
                if((aurMechMask & MECHANIC_NOT_REMOVED_BY_SHAPESHIFT) ||
                    // some Daze spells have these parameters instead of MECHANIC_DAZE (skip snare spells)
                    aurSpellInfo->SpellIconID == 15 && aurSpellInfo->Dispel == 0 &&
                    (aurMechMask & (1 << (MECHANIC_SNARE-1)))==0)
                {
                    ++iter;
                    continue;
                }

                // All OK, remove aura now
                m_target->RemoveAurasDueToSpellByCancel(aurSpellInfo->Id);
                iter = slowingAuras.begin();
            }

            // and polymorphic affects
            if(m_target->IsPolymorphed())
                m_target->RemoveAurasDueToSpell(m_target->getTransForm());

            break;
        }
        default:
           break;
    }

    if(apply)
    {
        // remove other shapeshift before applying a new one
        if(m_target->m_ShapeShiftFormSpellId)
            m_target->RemoveAurasDueToSpell(m_target->m_ShapeShiftFormSpellId, this);

        m_target->SetByteValue(UNIT_FIELD_BYTES_2, 3, form);

        if(modelid > 0)
            m_target->SetDisplayId(modelid);

        if(PowerType != POWER_MANA)
        {
            // reset power to default values only at power change
            if(m_target->getPowerType() != PowerType)
                m_target->setPowerType(PowerType);

            switch(form)
            {
                case FORM_CAT:
                case FORM_BEAR:
                case FORM_DIREBEAR:
                {
                    // get furor proc chance
                    int32 furorChance = 0;
                    Unit::AuraList const& mDummy = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator i = mDummy.begin(); i != mDummy.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->SpellIconID == 238)
                        {
                            furorChance = (*i)->GetModifier()->m_amount;
                            break;
                        }
                    }

                    if (m_modifier.m_miscvalue == FORM_CAT)
                    {
                        // Furor chance is now amount allowed to save energy for cat form
                        // without talent it reset to 0
                        if (m_target->GetPower(POWER_ENERGY) > furorChance)
                        {
                            m_target->SetPower(POWER_ENERGY, 0);
                            m_target->CastCustomSpell(m_target, 17099, &furorChance, NULL, NULL, this);
                        }
                    }
                    else if(furorChance)                    // only if talent known
                    {
                        m_target->SetPower(POWER_RAGE, 0);
                        if(urand(1,100) <= furorChance)
                            m_target->CastSpell(m_target, 17057, true, NULL, this);
                    }
                    break;
                }
                case FORM_BATTLESTANCE:
                case FORM_DEFENSIVESTANCE:
                case FORM_BERSERKERSTANCE:
                {
                    uint32 Rage_val = 0;
                    // Stance mastery + Tactical mastery (both passive, and last have aura only in defense stance, but need apply at any stance switch)
                    if(m_target->GetTypeId() == TYPEID_PLAYER)
                    {
                        PlayerSpellMap const& sp_list = ((Player *)m_target)->GetSpellMap();
                        for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
                        {
                            if(itr->second->state == PLAYERSPELL_REMOVED) continue;
                            SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                            if (spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_WARRIOR && spellInfo->SpellIconID == 139)
                                Rage_val += m_target->CalculateSpellDamage(spellInfo, 0, spellInfo->EffectBasePoints[0], m_target) * 10;
                        }
                    }

                    if (m_target->GetPower(POWER_RAGE) > Rage_val)
                        m_target->SetPower(POWER_RAGE, Rage_val);
                    break;
                }
                default:
                    break;
            }
        }

        m_target->m_ShapeShiftFormSpellId = GetId();
        m_target->m_form = form;

        // a form can give the player a new castbar with some spells.. this is a clientside process..
        // serverside just needs to register the new spells so that player isn't kicked as cheater
        if (m_target->GetTypeId() == TYPEID_PLAYER)
            for (uint32 i = 0; i < 8; ++i)
                if (ssEntry->spellId[i])
                    ((Player*)m_target)->addSpell(ssEntry->spellId[i], true, false, false, false);

    }
    else
    {
        if(modelid > 0)
            m_target->SetDisplayId(m_target->GetNativeDisplayId());
        m_target->SetByteValue(UNIT_FIELD_BYTES_2, 3, FORM_NONE);
        if(m_target->getClass() == CLASS_DRUID)
            m_target->setPowerType(POWER_MANA);
        m_target->m_ShapeShiftFormSpellId = 0;
        m_target->m_form = FORM_NONE;

        switch(form)
        {
            // Nordrassil Harness - bonus
            case FORM_BEAR:
            case FORM_DIREBEAR:
            case FORM_CAT:
                if(Aura* dummy = m_target->GetDummyAura(37315) )
                    m_target->CastSpell(m_target, 37316, true, NULL, dummy);
                break;
            // Nordrassil Regalia - bonus
            case FORM_MOONKIN:
                if(Aura* dummy = m_target->GetDummyAura(37324) )
                    m_target->CastSpell(m_target, 37325, true, NULL, dummy);
                break;
            default:
                break;
        }

        // look at the comment in apply-part
        if (m_target->GetTypeId() == TYPEID_PLAYER)
            for (uint32 i = 0; i < 8; ++i)
                if (ssEntry->spellId[i])
                    ((Player*)m_target)->removeSpell(ssEntry->spellId[i], false, false, false);

    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(apply);

    if(m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->InitDataForForm();
}

void Aura::HandleAuraTransform(bool apply, bool Real)
{
    if (apply)
    {
        // special case (spell specific functionality)
        if (m_modifier.m_miscvalue == 0)
        {
            // player applied only
            if (m_target->GetTypeId() != TYPEID_PLAYER)
                return;

            switch (GetId())
            {
                // Orb of Deception
                case 16739:
                {
                    uint32 orb_model = m_target->GetNativeDisplayId();
                    switch(orb_model)
                    {
                        // Troll Female
                        case 1479: m_target->SetDisplayId(10134); break;
                        // Troll Male
                        case 1478: m_target->SetDisplayId(10135); break;
                        // Tauren Male
                        case 59:   m_target->SetDisplayId(10136); break;
                        // Human Male
                        case 49:   m_target->SetDisplayId(10137); break;
                        // Human Female
                        case 50:   m_target->SetDisplayId(10138); break;
                        // Orc Male
                        case 51:   m_target->SetDisplayId(10139); break;
                        // Orc Female
                        case 52:   m_target->SetDisplayId(10140); break;
                        // Dwarf Male
                        case 53:   m_target->SetDisplayId(10141); break;
                        // Dwarf Female
                        case 54:   m_target->SetDisplayId(10142); break;
                        // NightElf Male
                        case 55:   m_target->SetDisplayId(10143); break;
                        // NightElf Female
                        case 56:   m_target->SetDisplayId(10144); break;
                        // Undead Female
                        case 58:   m_target->SetDisplayId(10145); break;
                        // Undead Male
                        case 57:   m_target->SetDisplayId(10146); break;
                        // Tauren Female
                        case 60:   m_target->SetDisplayId(10147); break;
                        // Gnome Male
                        case 1563: m_target->SetDisplayId(10148); break;
                        // Gnome Female
                        case 1564: m_target->SetDisplayId(10149); break;
                        // BloodElf Female
                        case 15475: m_target->SetDisplayId(17830); break;
                        // BloodElf Male
                        case 15476: m_target->SetDisplayId(17829); break;
                        // Dranei Female
                        case 16126: m_target->SetDisplayId(17828); break;
                        // Dranei Male
                        case 16125: m_target->SetDisplayId(17827); break;
                        default: break;
                    }
                    break;
                }
                // Murloc costume
                case 42365: m_target->SetDisplayId(21723); break;
                // Honor the Dead
                case 65386:
                case 65495:
                {
                    switch(m_target->getGender())
                    {
                        case GENDER_MALE:
                            m_target->SetDisplayId(29203);  // Chapman
                            break;
                        case GENDER_FEMALE:
                        case GENDER_NONE:
                            m_target->SetDisplayId(29204);  // Catrina
                            break;
                    }
                    break;
                }
                default: break;
            }
        }
        else
        {
            uint32 model_id;

            CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
            if (!ci)
            {
                model_id = 16358;                           // pig pink ^_^
                sLog.outError("Auras: unknown creature id = %d (only need its modelid) Form Spell Aura Transform in Spell ID = %d", m_modifier.m_miscvalue, GetId());
            }
            else
                model_id = ci->DisplayID_A[0];              // Will use the default model here

            // Polymorph (sheep/penguin case)
            if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_MAGE && GetSpellProto()->SpellIconID == 82)
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(52648))             // Glyph of the Penguin
                        model_id = 26452;

            m_target->SetDisplayId(model_id);

            // Dragonmaw Illusion (set mount model also)
            if(GetId()==42016 && m_target->GetMountID() && !m_target->GetAurasByType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED).empty())
                m_target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID,16314);
        }

        // update active transform spell only not set or not overwriting negative by positive case
        if (!m_target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(m_target->getTransForm()))
            m_target->setTransForm(GetId());

        // polymorph case
        if (Real && m_target->GetTypeId() == TYPEID_PLAYER && m_target->IsPolymorphed())
        {
            // for players, start regeneration after 1s (in polymorph fast regeneration case)
            // only if caster is Player (after patch 2.4.2)
            if (IS_PLAYER_GUID(GetCasterGUID()) )
                ((Player*)m_target)->setRegenTimer(1*IN_MILISECONDS);

            //dismount polymorphed target (after patch 2.4.2)
            if (m_target->IsMounted())
                m_target->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
        }
    }
    else
    {
        // ApplyModifier(true) will reapply it if need
        m_target->setTransForm(0);
        m_target->SetDisplayId(m_target->GetNativeDisplayId());

        // re-aplly some from still active with preference negative cases
        Unit::AuraList const& otherTransforms = m_target->GetAurasByType(SPELL_AURA_TRANSFORM);
        if (!otherTransforms.empty())
        {
            // look for other transform auras
            Aura* handledAura = *otherTransforms.begin();
            for(Unit::AuraList::const_iterator i = otherTransforms.begin();i != otherTransforms.end(); ++i)
            {
                // negative auras are preferred
                if (!IsPositiveSpell((*i)->GetSpellProto()->Id))
                {
                    handledAura = *i;
                    break;
                }
            }
            handledAura->ApplyModifier(true);
        }

        // Dragonmaw Illusion (restore mount model)
        if (GetId() == 42016 && m_target->GetMountID() == 16314)
        {
            if (!m_target->GetAurasByType(SPELL_AURA_MOUNTED).empty())
            {
                uint32 cr_id = m_target->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetModifier()->m_miscvalue;
                if (CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(cr_id))
                {
                    uint32 team = 0;
                    if (m_target->GetTypeId() == TYPEID_PLAYER)
                        team = ((Player*)m_target)->GetTeam();

                    uint32 display_id = sObjectMgr.ChooseDisplayId(team, ci);
                    CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
                    if (minfo)
                        display_id = minfo->modelid;

                    m_target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, display_id);
                }
            }
        }
    }
}

void Aura::HandleForceReaction(bool apply, bool Real)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(!Real)
        return;

    Player* player = (Player*)m_target;

    uint32 faction_id = m_modifier.m_miscvalue;
    ReputationRank faction_rank = ReputationRank(m_modifier.m_amount);

    player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
    player->GetReputationMgr().SendForceReactions();

    // stop fighting if at apply forced rank friendly or at remove real rank friendly
    if (apply && faction_rank >= REP_FRIENDLY || !apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY)
        player->StopAttackFaction(faction_id);
}

void Aura::HandleAuraModSkill(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 prot=GetSpellProto()->EffectMiscValue[m_effIndex];
    int32 points = GetModifier()->m_amount;

    ((Player*)m_target)->ModifySkillBonus(prot, (apply ? points: -points), m_modifier.m_auraname == SPELL_AURA_MOD_SKILL_TALENT);
    if(prot == SKILL_DEFENSE)
        ((Player*)m_target)->UpdateDefenseBonusesMod();
}

void Aura::HandleChannelDeathItem(bool apply, bool Real)
{
    if(Real && !apply)
    {
        if(m_removeMode != AURA_REMOVE_BY_DEATH)
            return;
        // Item amount
        if (m_modifier.m_amount <= 0)
            return;

        SpellEntry const *spellInfo = GetSpellProto();
        if(spellInfo->EffectItemType[m_effIndex] == 0)
            return;

        Unit* victim = GetTarget();
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;

        // Soul Shard only from non-grey units
        if( spellInfo->EffectItemType[m_effIndex] == 6265 &&
            (victim->getLevel() <= MaNGOS::XP::GetGrayLevel(caster->getLevel()) ||
             victim->GetTypeId()==TYPEID_UNIT && !((Player*)caster)->isAllowedToLoot((Creature*)victim)) )
            return;
        //Adding items
        uint32 noSpaceForCount = 0;
        uint32 count = m_modifier.m_amount;

        ItemPosCountVec dest;
        uint8 msg = ((Player*)caster)->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, spellInfo->EffectItemType[m_effIndex], count, &noSpaceForCount);
        if( msg != EQUIP_ERR_OK )
        {
            count-=noSpaceForCount;
            ((Player*)caster)->SendEquipError( msg, NULL, NULL );
            if (count==0)
                return;
        }

        Item* newitem = ((Player*)caster)->StoreNewItem(dest, spellInfo->EffectItemType[m_effIndex], true);
        ((Player*)caster)->SendNewItem(newitem, count, true, false);
    }
}

void Aura::HandleBindSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)caster)->SetFarSightGUID(apply ? m_target->GetGUID() : 0);
}

void Aura::HandleFarSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)caster)->SetFarSightGUID(apply ? m_target->GetGUID() : 0);
}

void Aura::HandleAuraTrackCreatures(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        m_target->RemoveNoStackAurasDueToAura(this);
    m_target->SetUInt32Value(PLAYER_TRACK_CREATURES, apply ? ((uint32)1)<<(m_modifier.m_miscvalue-1) : 0 );
}

void Aura::HandleAuraTrackResources(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        m_target->RemoveNoStackAurasDueToAura(this);
    m_target->SetUInt32Value(PLAYER_TRACK_RESOURCES, apply ? ((uint32)1)<<(m_modifier.m_miscvalue-1): 0 );
}

void Aura::HandleAuraTrackStealthed(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        m_target->RemoveNoStackAurasDueToAura(this);

    m_target->ApplyModFlag(PLAYER_FIELD_BYTES, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void Aura::HandleAuraModScale(bool apply, bool /*Real*/)
{
    m_target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, m_modifier.m_amount, apply);
}

void Aura::HandleModPossess(bool apply, bool Real)
{
    if(!Real)
        return;

    // not possess yourself
    if(GetCasterGUID() == m_target->GetGUID())
        return;

    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)caster;


    if( apply )
    {
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        m_target->SetCharmerGUID(p_caster->GetGUID());
        m_target->setFaction(p_caster->getFaction());

        p_caster->SetCharm(m_target);

        p_caster->SetFarSightGUID(m_target->GetGUID());
        p_caster->SetClientControl(m_target, 1);
        p_caster->SetMover(m_target);

        m_target->CombatStop();
        m_target->DeleteThreatList();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            m_target->StopMoving();
            m_target->GetMotionMaster()->Clear();
            m_target->GetMotionMaster()->MoveIdle();
        }
        else if(m_target->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)m_target)->SetClientControl(m_target, 0);
        }

        if(CharmInfo *charmInfo = m_target->InitCharmInfo(m_target))
            charmInfo->InitPossessCreateSpells();

        p_caster->PossessSpellInitialize();
    }
    else
    {
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        m_target->SetCharmerGUID(0);
        p_caster->InterruptSpell(CURRENT_CHANNELED_SPELL);  // the spell is not automatically canceled when interrupted, do it now

        if(m_target->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)m_target)->setFactionForRace(m_target->getRace());
            ((Player*)m_target)->SetClientControl(m_target, 1);
        }
        else if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            CreatureInfo const *cinfo = ((Creature*)m_target)->GetCreatureInfo();
            m_target->setFaction(cinfo->faction_A);
        }

        p_caster->SetCharm(NULL);

        p_caster->SetFarSightGUID(0);
        p_caster->SetClientControl(m_target, 0);
        p_caster->SetMover(NULL);

        p_caster->RemovePetActionBar();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)m_target)->AIM_Initialize();

            if (((Creature*)m_target)->AI())
                ((Creature*)m_target)->AI()->AttackedBy(caster);
        }
    }
}

void Aura::HandleModPossessPet(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Pet *pet = caster->GetPet();
    if(!pet || pet != m_target)
        return;

    Player* p_caster = (Player*)caster;

    if(apply)
        pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
    else
        pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

    p_caster->SetFarSightGUID(apply ? pet->GetGUID() : 0);
    p_caster->SetCharm(apply ? pet : NULL);
    p_caster->SetClientControl(pet, apply ? 1 : 0);
    ((Player*)caster)->SetMover(apply ? pet : NULL);

    if(apply)
    {
        pet->StopMoving();
        pet->GetMotionMaster()->Clear();
        pet->GetMotionMaster()->MoveIdle();
    }
    else
    {
        pet->AttackStop();
        pet->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        pet->AddMonsterMoveFlag(MONSTER_MOVE_WALK);
    }
}

void Aura::HandleAuraModPetTalentsPoints(bool /*Apply*/, bool Real)
{
    if(!Real)
        return;

    // Recalculate pet talent points
    if (Pet *pet=m_target->GetPet())
        pet->InitTalentForLevel();
}

void Aura::HandleModCharm(bool apply, bool Real)
{
    if(!Real)
        return;

    // not charm yourself
    if(GetCasterGUID() == m_target->GetGUID())
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    if( apply )
    {
        if (m_target->GetCharmerGUID())
        {
            m_target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM);
            m_target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS);
        }

        m_target->SetCharmerGUID(GetCasterGUID());
        m_target->setFaction(caster->getFaction());
        m_target->CastStop(m_target == caster ? GetId() : 0);
        caster->SetCharm(m_target);

        m_target->CombatStop();
        m_target->DeleteThreatList();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)m_target)->AIM_Initialize();
            CharmInfo *charmInfo = m_target->InitCharmInfo(m_target);
            charmInfo->InitCharmCreateSpells();
            charmInfo->SetReactState( REACT_DEFENSIVE );

            if(caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK)
            {
                CreatureInfo const *cinfo = ((Creature*)m_target)->GetCreatureInfo();
                if(cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                {
                    // creature with pet number expected have class set
                    if(m_target->GetByteValue(UNIT_FIELD_BYTES_0, 1)==0)
                    {
                        if(cinfo->unit_class==0)
                            sLog.outErrorDb("Creature (Entry: %u) have unit_class = 0 but used in charmed spell, that will be result client crash.",cinfo->Entry);
                        else
                            sLog.outError("Creature (Entry: %u) have unit_class = %u but at charming have class 0!!! that will be result client crash.",cinfo->Entry,cinfo->unit_class);

                        m_target->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
                    }

                    //just to enable stat window
                    charmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
                    //if charmed two demons the same session, the 2nd gets the 1st one's name
                    m_target->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, time(NULL));
                }
            }
        }

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->CharmSpellInitialize();
    }
    else
    {
        m_target->SetCharmerGUID(0);

        if(m_target->GetTypeId() == TYPEID_PLAYER)
            ((Player*)m_target)->setFactionForRace(m_target->getRace());
        else
        {
            CreatureInfo const *cinfo = ((Creature*)m_target)->GetCreatureInfo();

            // restore faction
            if(((Creature*)m_target)->isPet())
            {
                if(Unit* owner = m_target->GetOwner())
                    m_target->setFaction(owner->getFaction());
                else if(cinfo)
                    m_target->setFaction(cinfo->faction_A);
            }
            else if(cinfo)                              // normal creature
                m_target->setFaction(cinfo->faction_A);

            // restore UNIT_FIELD_BYTES_0
            if(cinfo && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK && cinfo->type == CREATURE_TYPE_DEMON)
            {
                // DB must have proper class set in field at loading, not req. restore, including workaround case at apply
                // m_target->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->unit_class);

                if(m_target->GetCharmInfo())
                    m_target->GetCharmInfo()->SetPetNumber(0, true);
                else
                    sLog.outError("Aura::HandleModCharm: target (GUID: %u TypeId: %u) has a charm aura but no charm info!", m_target->GetGUIDLow(), m_target->GetTypeId());
            }
        }

        caster->SetCharm(NULL);

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->RemovePetActionBar();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)m_target)->AIM_Initialize();
            if (((Creature*)m_target)->AI())
                ((Creature*)m_target)->AI()->AttackedBy(caster);
        }
    }
}

void Aura::HandleModConfuse(bool apply, bool Real)
{
    if(!Real)
        return;

    m_target->SetConfused(apply, GetCasterGUID(), GetId());
}

void Aura::HandleModFear(bool apply, bool Real)
{
    if (!Real)
        return;

    m_target->SetFeared(apply, GetCasterGUID(), GetId());
}

void Aura::HandleFeignDeath(bool apply, bool Real)
{
    if(!Real)
        return;

    m_target->SetFeignDeath(apply, GetCasterGUID(), GetId());
}

void Aura::HandleAuraModDisarm(bool apply, bool Real)
{
    if(!Real)
        return;

    if(!apply && m_target->HasAuraType(SPELL_AURA_MOD_DISARM))
        return;

    // not sure for it's correctness
    if(apply)
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED);
    else
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED);

    // only at real add/remove aura
    if (m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // main-hand attack speed already set to special value for feral form already and don't must change and reset at remove.
    if (m_target->IsInFeralForm())
        return;

    if (apply)
        m_target->SetAttackTime(BASE_ATTACK,BASE_ATTACK_TIME);
    else
        ((Player *)m_target)->SetRegularAttackTime();

    m_target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleAuraModStun(bool apply, bool Real)
{
    if(!Real)
        return;

    if (apply)
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(m_spellProto) & SPELL_SCHOOL_MASK_FROST)
            m_target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        m_target->addUnitState(UNIT_STAT_STUNNED);
        m_target->SetTargetGUID(0);

        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        m_target->CastStop(m_target->GetGUID() == GetCasterGUID() ? GetId() : 0);

        // Creature specific
        if(m_target->GetTypeId() != TYPEID_PLAYER)
            m_target->StopMoving();
        else
        {
            ((Player*)m_target)->m_movementInfo.SetMovementFlags(MOVEMENTFLAG_NONE);
            m_target->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client
        }

        WorldPacket data(SMSG_FORCE_MOVE_ROOT, 8);
        data.append(m_target->GetPackGUID());
        data << uint32(0);
        m_target->SendMessageToSet(&data, true);

        // Summon the Naj'entus Spine GameObject on target if spell is Impaling Spine
        if(GetId() == 39837)
        {
            GameObject* pObj = new GameObject;
            if(pObj->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_GAMEOBJECT), 185584, m_target->GetMap(), m_target->GetPhaseMask(),
                m_target->GetPositionX(), m_target->GetPositionY(), m_target->GetPositionZ(), m_target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
            {
                pObj->SetRespawnTime(GetAuraDuration()/IN_MILISECONDS);
                pObj->SetSpellId(GetId());
                m_target->AddGameObject(pObj);
                m_target->GetMap()->Add(pObj);
            }
            else
                delete pObj;
        }
    }
    else
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(m_spellProto) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for(AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = m_target->GetAurasByType(*itr);
                for(Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if( GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if(found_another)
                    break;
            }

            if(!found_another)
                m_target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if(m_target->HasAuraType(SPELL_AURA_MOD_STUN))
            return;

        m_target->clearUnitState(UNIT_STAT_STUNNED);
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if(!m_target->hasUnitState(UNIT_STAT_ROOT))         // prevent allow move if have also root effect
        {
            if(m_target->getVictim() && m_target->isAlive())
                m_target->SetTargetGUID(m_target->getVictim()->GetGUID());

            WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8+4);
            data.append(m_target->GetPackGUID());
            data << uint32(0);
            m_target->SendMessageToSet(&data, true);
        }

        // Wyvern Sting
        if (m_spellProto->SpellFamilyName == SPELLFAMILY_HUNTER && m_spellProto->SpellFamilyFlags & UI64LIT(0x0000100000000000))
        {
            Unit* caster = GetCaster();
            if( !caster || caster->GetTypeId()!=TYPEID_PLAYER )
                return;

            uint32 spell_id = 0;

            switch(GetId())
            {
                case 19386: spell_id = 24131; break;
                case 24132: spell_id = 24134; break;
                case 24133: spell_id = 24135; break;
                case 27068: spell_id = 27069; break;
                case 49011: spell_id = 49009; break;
                case 49012: spell_id = 49010; break;
                default:
                    sLog.outError("Spell selection called for unexpected original spell %u, new spell for this spell family?",GetId());
                    return;
            }

            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);

            if(!spellInfo)
                return;

            caster->CastSpell(m_target,spellInfo,true,NULL,this);
            return;
        }
    }
}

void Aura::HandleModStealth(bool apply, bool Real)
{
    if (apply)
    {
        // drop flag at stealth in bg
         m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // only at real aura add
        if (Real)
        {
            m_target->SetStandFlags(UNIT_STAND_FLAGS_CREEP);

            if (m_target->GetTypeId()==TYPEID_PLAYER)
                m_target->SetFlag(PLAYER_FIELD_BYTES2, 0x2000);

            // apply only if not in GM invisibility (and overwrite invisibility state)
            if (m_target->GetVisibility()!=VISIBILITY_OFF)
            {
                m_target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                m_target->SetVisibility(VISIBILITY_GROUP_STEALTH);
            }

            // apply full stealth period bonuses only at first stealth aura in stack
            if(m_target->GetAurasByType(SPELL_AURA_MOD_STEALTH).size()<=1)
            {
                Unit::AuraList const& mDummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = mDummyAuras.begin();i != mDummyAuras.end(); ++i)
                {
                    // Master of Subtlety
                    if ((*i)->GetSpellProto()->SpellIconID == 2114)
                    {
                        m_target->RemoveAurasDueToSpell(31666);
                        int32 bp = (*i)->GetModifier()->m_amount;
                        m_target->CastCustomSpell(m_target,31665,&bp,NULL,NULL,true);
                    }
                    // Overkill
                    else if ((*i)->GetId() == 58426 && GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000400000))
                    {
                        m_target->RemoveAurasDueToSpell(58428);
                        m_target->CastSpell(m_target, 58427, true);
                    }
                }
            }
        }
    }
    else
    {
        // only at real aura remove of _last_ SPELL_AURA_MOD_STEALTH
        if (Real && !m_target->HasAuraType(SPELL_AURA_MOD_STEALTH))
        {
            // if no GM invisibility
            if (m_target->GetVisibility()!=VISIBILITY_OFF)
            {
                m_target->RemoveStandFlags(UNIT_STAND_FLAGS_CREEP);

                if (m_target->GetTypeId()==TYPEID_PLAYER)
                    m_target->RemoveFlag(PLAYER_FIELD_BYTES2, 0x2000);

                // restore invisibility if any
                if (m_target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
                {
                    m_target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                    m_target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
                }
                else
                    m_target->SetVisibility(VISIBILITY_ON);
            }

            // apply delayed talent bonus remover at last stealth aura remove
            Unit::AuraList const& mDummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
            for(Unit::AuraList::const_iterator i = mDummyAuras.begin();i != mDummyAuras.end(); ++i)
            {
                // Master of Subtlety
                if ((*i)->GetSpellProto()->SpellIconID == 2114)
                    m_target->CastSpell(m_target, 31666, true);
                // Overkill
                else if ((*i)->GetId() == 58426 && GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000400000))
                    m_target->CastSpell(m_target, 58428, true);
            }
        }
    }
}

void Aura::HandleInvisibility(bool apply, bool Real)
{
    if(apply)
    {
        m_target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);

         m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        if(Real && m_target->GetTypeId()==TYPEID_PLAYER)
        {
            // apply glow vision
            m_target->SetFlag(PLAYER_FIELD_BYTES2,PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

        }

        // apply only if not in GM invisibility and not stealth
        if(m_target->GetVisibility() == VISIBILITY_ON)
        {
            // Aura not added yet but visibility code expect temporary add aura
            m_target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
            m_target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        m_target->m_invisibilityMask = 0;
        Unit::AuraList const& auras = m_target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            m_target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);

        // only at real aura remove and if not have different invisibility auras.
        if(Real && m_target->m_invisibilityMask == 0)
        {
            // remove glow vision
            if(m_target->GetTypeId() == TYPEID_PLAYER)
                m_target->RemoveFlag(PLAYER_FIELD_BYTES2,PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

            // apply only if not in GM invisibility & not stealthed while invisible
            if(m_target->GetVisibility() != VISIBILITY_OFF)
            {
                // if have stealth aura then already have stealth visibility
                if(!m_target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                    m_target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibilityDetect(bool apply, bool Real)
{
    if(apply)
    {
        m_target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        m_target->m_detectInvisibilityMask = 0;
        Unit::AuraList const& auras = m_target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            m_target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    }
    if(Real && m_target->GetTypeId()==TYPEID_PLAYER)
        ((Player*)m_target)->UpdateVisibilityForPlayer();
}

void Aura::HandleAuraModRoot(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    if (apply)
    {
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(m_spellProto) & SPELL_SCHOOL_MASK_FROST)
            m_target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        m_target->addUnitState(UNIT_STAT_ROOT);
        m_target->SetTargetGUID(0);
        // probably wrong (this add skinnable flag)
        // TODO: find correct flag
        //m_target->SetFlag(UNIT_FIELD_FLAGS,(apply_stat<<16));

        //Save last orientation
        if( m_target->getVictim() )
            m_target->SetOrientation(m_target->GetAngle(m_target->getVictim()));

        if(m_target->GetTypeId() == TYPEID_PLAYER)
        {
            WorldPacket data(SMSG_FORCE_MOVE_ROOT, 10);
            data.append(m_target->GetPackGUID());
            data << (uint32)2;
            m_target->SendMessageToSet(&data, true);

            //Clear unit movement flags
            ((Player*)m_target)->m_movementInfo.SetMovementFlags(MOVEMENTFLAG_NONE);
        }
        else
            m_target->StopMoving();
    }
    else
    {
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(m_spellProto) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for(AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = m_target->GetAurasByType(*itr);
                for(Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if( GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if(found_another)
                    break;
            }

            if(!found_another)
                m_target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if(m_target->HasAuraType(SPELL_AURA_MOD_ROOT))
            return;

        m_target->clearUnitState(UNIT_STAT_ROOT);
        // probably wrong (this add skinnable flag)
        // TODO: find correct flag
        //m_target->RemoveFlag(UNIT_FIELD_FLAGS,(apply_stat<<16));

        if(!m_target->hasUnitState(UNIT_STAT_STUNNED))      // prevent allow move if have also stun effect
        {
            if(m_target->getVictim() && m_target->isAlive())
                m_target->SetTargetGUID(m_target->getVictim()->GetGUID());

            if(m_target->GetTypeId() == TYPEID_PLAYER)
            {
                WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 10);
                data.append(m_target->GetPackGUID());
                data << (uint32)2;
                m_target->SendMessageToSet(&data, true);
            }
        }
    }
}

void Aura::HandleAuraModSilence(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    if(apply)
    {
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = m_target->GetCurrentSpell(CurrentSpellTypes(i)))
                if(spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    m_target->InterruptSpell(CurrentSpellTypes(i), false);
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if(m_target->HasAuraType(SPELL_AURA_MOD_SILENCE))
            return;

        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void Aura::HandleModThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    if (!m_target->isAlive())
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    int level_diff = 0;
    int multiplier = 0;
    switch (GetId())
    {
        // Arcane Shroud
        case 26400:
            level_diff = m_target->getLevel() - 60;
            multiplier = 2;
            break;
        // The Eye of Diminution
        case 28862:
            level_diff = m_target->getLevel() - 60;
            multiplier = 1;
            break;
    }

    if (level_diff > 0)
        m_modifier.m_amount += multiplier * level_diff;

    if (m_target->GetTypeId() == TYPEID_PLAYER)
        for(int8 x=0;x < MAX_SPELL_SCHOOL;x++)
            if (m_modifier.m_miscvalue & int32(1<<x))
                ApplyPercentModFloatVar(m_target->m_threatModifier[x], m_modifier.m_amount, apply);
}

void Aura::HandleAuraModTotalThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    if (!m_target->isAlive() || m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    float threatMod = apply ? float(m_modifier.m_amount) : float(-m_modifier.m_amount);

    m_target->getHostileRefManager().threatAssist(caster, threatMod, GetSpellProto());
}

void Aura::HandleModTaunt(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    if (!m_target->isAlive() || !m_target->CanHaveThreatList())
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    if (apply)
        m_target->TauntApply(caster);
    else
    {
        // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
        m_target->TauntFadeOut(caster);
    }
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void Aura::HandleAuraModIncreaseSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseMountedSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_RUN, true);

    // Festive Holiday Mount
    if (apply && GetSpellProto()->SpellIconID != 1794 && m_target->HasAura(62061))
        // Reindeer Transformation
        m_target->CastSpell(m_target, 25860, true, NULL, this);
}

void Aura::HandleAuraModIncreaseFlightSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    // Enable Fly mode for flying mounts
    if (m_modifier.m_auraname == SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)
    {
        WorldPacket data;
        if(apply)
            data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
        else
            data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
        data.append(m_target->GetPackGUID());
        data << uint32(0);                                      // unknown
        m_target->SendMessageToSet(&data, true);

        //Players on flying mounts must be immune to polymorph
        if (m_target->GetTypeId()==TYPEID_PLAYER)
            m_target->ApplySpellImmune(GetId(),IMMUNITY_MECHANIC,MECHANIC_POLYMORPH,apply);

        // Dragonmaw Illusion (overwrite mount model, mounted aura already applied)
        if( apply && m_target->HasAura(42016,0) && m_target->GetMountID())
            m_target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID,16314);

        // Festive Holiday Mount
        if (apply && GetSpellProto()->SpellIconID != 1794 && m_target->HasAura(62061))
            // Reindeer Transformation
            m_target->CastSpell(m_target, 25860, true, NULL, this);
    }

    m_target->UpdateSpeed(MOVE_FLIGHT, true);
}

void Aura::HandleAuraModIncreaseSwimSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModDecreaseSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    if (apply)
    {
        // Gronn Lord's Grasp, becomes stoned
        if (GetId() == 33572)
        {
            if (GetStackAmount() >= 5 && !m_target->HasAura(33652))
                m_target->CastSpell(m_target, 33652, true);
        }
    }

    m_target->UpdateSpeed(MOVE_RUN, true);
    m_target->UpdateSpeed(MOVE_SWIM, true);
    m_target->UpdateSpeed(MOVE_FLIGHT, true);
}

void Aura::HandleAuraModUseNormalSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_RUN, true);
    m_target->UpdateSpeed(MOVE_SWIM, true);
    m_target->UpdateSpeed(MOVE_FLIGHT, true);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void Aura::HandleModMechanicImmunity(bool apply, bool /*Real*/)
{
    uint32 misc  = m_modifier.m_miscvalue;
    // Forbearance
    // in DBC wrong mechanic immune since 3.0.x
    if (GetId() == 25771)
        misc = MECHANIC_IMMUNE_SHIELD;


    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        uint32 mechanic = 1 << (misc-1);

        //immune movement impairment and loss of control
        if(GetId()==42292 || GetId()==59752)
            mechanic=IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

        m_target->RemoveAurasAtMechanicImmunity(mechanic,GetId());
    }

    m_target->ApplySpellImmune(GetId(),IMMUNITY_MECHANIC,misc,apply);

    // Bestial Wrath
    if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellIconID == 1680)
    {
        // The Beast Within cast on owner if talent present
        if (Unit* owner = m_target->GetOwner())
        {
            // Search talent
            Unit::AuraList const& dummyAuras = owner->GetAurasByType(SPELL_AURA_DUMMY);
            for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
            {
                if ((*i)->GetSpellProto()->SpellIconID == 2229)
                {
                    if (apply)
                        owner->CastSpell(owner, 34471, true, 0, this);
                    else
                        owner->RemoveAurasDueToSpell(34471);
                    break;
                }
            }
        }
    }
}

void Aura::HandleModMechanicImmunityMask(bool apply, bool /*Real*/)
{
    uint32 mechanic  = m_modifier.m_miscvalue;

    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        m_target->RemoveAurasAtMechanicImmunity(mechanic,GetId());

    // check implemented in Unit::IsImmunedToSpell and Unit::IsImmunedToSpellEffect
}

//this method is called whenever we add / remove aura which gives m_target some imunity to some spell effect
void Aura::HandleAuraModEffectImmunity(bool apply, bool /*Real*/)
{
    // when removing flag aura, handle flag drop
    if( !apply && m_target->GetTypeId() == TYPEID_PLAYER
        && (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION) )
    {
        if( BattleGround *bg = ((Player*)m_target)->GetBattleGround() )
            bg->EventPlayerDroppedFlag(((Player*)m_target));
    }

    m_target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModStateImmunity(bool apply, bool Real)
{
    if(apply && Real && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        Unit::AuraList const& auraList = m_target->GetAurasByType(AuraType(m_modifier.m_miscvalue));
        for(Unit::AuraList::const_iterator itr = auraList.begin(); itr != auraList.end();)
        {
            if (auraList.front() != this)                   // skip itself aura (it already added)
            {
                m_target->RemoveAurasDueToSpell(auraList.front()->GetId());
                itr = auraList.begin();
            }
            else
                ++itr;
        }
    }

    m_target->ApplySpellImmune(GetId(), IMMUNITY_STATE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModSchoolImmunity(bool apply, bool Real)
{
    m_target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, m_modifier.m_miscvalue, apply);

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if( this->GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
        && this->GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD )
        m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if( Real && apply
        && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
        && IsPositiveSpell(GetId()) )                       //Only positive immunity removes auras
    {
        uint32 school_mask = m_modifier.m_miscvalue;
        Unit::AuraMap& Auras = m_target->GetAuras();
        for(Unit::AuraMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
        {
            next = iter;
            ++next;
            SpellEntry const *spell = iter->second->GetSpellProto();
            if((GetSpellSchoolMask(spell) & school_mask)//Check for school mask
                && !( spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)   //Spells unaffected by invulnerability
                && !iter->second->IsPositive()          //Don't remove positive spells
                && spell->Id != GetId() )               //Don't remove self
            {
                m_target->RemoveAurasDueToSpell(spell->Id);
                if(Auras.empty())
                    break;
                else
                    next = Auras.begin();
            }
        }
    }
    if( Real && GetSpellProto()->Mechanic == MECHANIC_BANISH )
    {
        if( apply )
            m_target->addUnitState(UNIT_STAT_ISOLATED);
        else
            m_target->clearUnitState(UNIT_STAT_ISOLATED);
    }
}

void Aura::HandleAuraModDmgImmunity(bool apply, bool /*Real*/)
{
    m_target->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModDispelImmunity(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->ApplySpellDispelImmunity(m_spellProto, DispelType(m_modifier.m_miscvalue), apply);
}

void Aura::HandleAuraProcTriggerSpell(bool apply, bool Real)
{
    if(!Real)
        return;

    if(apply)
    {
        // some spell have charges by functionality not have its in spell data
        switch (GetId())
        {
            case 28200:                                     // Ascendance (Talisman of Ascendance trinket)
                SetAuraCharges(6);
                break;
            default: break;
        }
    }
}

void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if(apply)
        m_target->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    else
        m_target->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
}

/*********************************************************/
/***                   PERIODIC                        ***/
/*********************************************************/

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    if (!apply)
    {
        switch(m_spellProto->Id)
        {
            case 66:                                        // Invisibility
                if (m_removeMode == AURA_REMOVE_BY_DEFAULT && m_duration<=0)
                    m_target->CastSpell(m_target, 32612, true, NULL, this);

                return;
            case 42783:                                     //Wrath of the Astrom...
                if (m_removeMode == AURA_REMOVE_BY_DEFAULT && GetEffIndex() + 1 < 3)
                    m_target->CastSpell(m_target, m_spellProto->CalculateSimpleValue(GetEffIndex()+1), true);
                return;
            default:
                break;
        }
    }
}

void Aura::HandlePeriodicTriggerSpellWithValue(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicEnergize(bool apply, bool Real)
{
    if (!Real)
        return;

    // For prevent double apply bonuses
    bool loading = (m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading());

    if (apply && !loading)
    {
        switch (GetId())
        {
            case 54833:                                     // Glyph of Innervate (value%/2 of casters base mana)
            {
                if (Unit* caster = GetCaster())
                    m_modifier.m_amount = int32(caster->GetCreateMana() * GetBasePoints() / (200 * m_maxduration / m_periodicTimer));
                break;

            }
            case 29166:                                     // Innervate (value% of casters base mana)
            {
                if (Unit* caster = GetCaster())
                {
                    // Glyph of Innervate
                    if (caster->HasAura(54832))
                        caster->CastSpell(caster,54833,true,NULL,this);

                    m_modifier.m_amount = int32(caster->GetCreateMana() * GetBasePoints() / (100 * m_maxduration / m_periodicTimer));
                }
                break;
            }
            case 48391:                                     // Owlkin Frenzy 2% base mana
                m_modifier.m_amount = m_target->GetCreateMana() * 2 / 100;
                break;
            case 57669:                                     // Replenishment (0.2% from max)
            case 61782:                                     // Infinite Replenishment
                m_modifier.m_amount = m_target->GetMaxPower(POWER_MANA) * 2 / 1000;
                break;
            default:
                break;
        }
    }

    m_isPeriodic = apply;
}

void Aura::HandleAuraPowerBurn(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandleAuraPeriodicDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // For prevent double apply bonuses
    bool loading = (m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading());

    SpellEntry const*spell = GetSpellProto();
    switch( spell->SpellFamilyName)
    {
        case SPELLFAMILY_ROGUE:
        {
            if(!apply)
            {
                switch(spell->Id)
                {
                    // Master of Subtlety
                    case 31666: m_target->RemoveAurasDueToSpell(31665); break;
                    // Overkill
                    case 58428: m_target->RemoveAurasDueToSpell(58427); break;
                }
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            Unit* caster = GetCaster();

            // Explosive Shot
            if (apply && !loading && caster)
                m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 14 / 100);
            break;
        }
    }

    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHeal(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        switch (m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                // Pounce Bleed
                if ( m_spellProto->SpellIconID == 147 && m_spellProto->SpellVisual[0] == 0 )
                {
                    // $AP*0.18/6 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 3 / 100);
                    return;
                }
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                // Rend
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000020))
                {
                    // $0.2*(($MWB+$mwb)/2+$AP/14*$MWS) bonus per tick
                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 mws = caster->GetAttackTime(BASE_ATTACK);
                    float mwb_min = caster->GetWeaponDamageRange(BASE_ATTACK,MINDAMAGE);
                    float mwb_max = caster->GetWeaponDamageRange(BASE_ATTACK,MAXDAMAGE);
                    m_modifier.m_amount+=int32(((mwb_min+mwb_max)/2+ap*mws/14000)*0.2f);
                    return;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                // Rake
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000001000) && m_spellProto->Effect[2]==SPELL_EFFECT_ADD_COMBO_POINTS)
                {
                    // $AP*0.18/3 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 6 / 100);
                    return;
                }
                // Lacerate
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000010000000000))
                {
                    // $AP*0.05/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                    return;
                }
                // Rip
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000000000800000))
                {
                    // 0.01*$AP*cp
                    if (caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    uint8 cp = ((Player*)caster)->GetComboPoints();

                    // Idol of Feral Shadows. Cant be handled as SpellMod in SpellAura:Dummy due its dependency from CPs
                    Unit::AuraList const& dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                    {
                        if((*itr)->GetId()==34241)
                        {
                            m_modifier.m_amount += cp * (*itr)->GetModifier()->m_amount;
                            break;
                        }
                    }
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                    return;
                }
                // Lock Jaw
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x1000000000000000))
                {
                    // 0.15*$AP
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 15 / 100);
                    return;
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Rupture
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000000000100000))
                {
                    if (caster->GetTypeId() != TYPEID_PLAYER)
                        return;
                    //1 point : ${($m1+$b1*1+0.015*$AP)*4} damage over 8 secs
                    //2 points: ${($m1+$b1*2+0.024*$AP)*5} damage over 10 secs
                    //3 points: ${($m1+$b1*3+0.03*$AP)*6} damage over 12 secs
                    //4 points: ${($m1+$b1*4+0.03428571*$AP)*7} damage over 14 secs
                    //5 points: ${($m1+$b1*5+0.0375*$AP)*8} damage over 16 secs
                    float AP_per_combo[6] = {0.0f, 0.015f, 0.024f, 0.03f, 0.03428571f, 0.0375f};
                    uint8 cp = ((Player*)caster)->GetComboPoints();
                    if (cp > 5) cp = 5;
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * AP_per_combo[cp]);
                    return;
                }
                // Garrote
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000000000000100))
                {
                    // $AP*0.07 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 7 / 100);
                    return;
                }
                // Deadly Poison
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000010000))
                {
                    // 0.08*$AP / 4 * amount of stack
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 2 * GetStackAmount() / 100);
                    return;
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                // Serpent Sting
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000004000))
                {
                    // $RAP*0.2/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 0.2 / 5);
                    return;
                }
                // Immolation Trap
                if ((m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000004)) && m_spellProto->SpellIconID == 678)
                {
                    // $RAP*0.1/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 10 / 500);
                    return;
                }
                break;
            }
            default:
                break;
        }
    }
    // remove time effects
    else
    {
        // Parasitic Shadowfiend - handle summoning of two Shadowfiends on DoT expire
        if(m_spellProto->Id == 41917)
            m_target->CastSpell(m_target, 41915, true);
    }
}

void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHealthFunnel(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

void Aura::HandleAuraModResistanceExclusive(bool apply, bool /*Real*/)
{
    for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
    {
        if(m_modifier.m_miscvalue & int32(1<<x))
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER)
                m_target->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, m_modifier.m_amount, apply);
        }
    }
}

void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
    for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
    {
        if(m_modifier.m_miscvalue & int32(1<<x))
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER || ((Creature*)m_target)->isPet())
                m_target->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, m_modifier.m_amount, apply);
        }
    }
}

void Aura::HandleAuraModBaseResistancePCT(bool apply, bool /*Real*/)
{
    // only players have base stats
    if(m_target->GetTypeId() != TYPEID_PLAYER)
    {
        //pets only have base armor
        if(((Creature*)m_target)->isPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            m_target->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(m_modifier.m_amount), apply);
    }
    else
    {
        for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
        {
            if(m_modifier.m_miscvalue & int32(1<<x))
                m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
    for(int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
    {
        if(m_modifier.m_miscvalue & int32(1<<i))
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER || ((Creature*)m_target)->isPet())
            {
                m_target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, m_modifier.m_amount, apply);
                m_target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, m_modifier.m_amount, apply);
            }
        }
    }
}

void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
    // only players have base stats
    if(m_target->GetTypeId() != TYPEID_PLAYER)
    {
        //only pets have base stats
        if(((Creature*)m_target)->isPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            m_target->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
    else
    {
        for(int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
            if(m_modifier.m_miscvalue & (1<<i))
                m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
}

/********************************/
/***           STAT           ***/
/********************************/

void Aura::HandleAuraModStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -2 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ",GetId(),GetEffIndex(),m_modifier.m_miscvalue);
        return;
    }

    for(int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        // -1 or -2 is all stats ( misc < -2 checked in function beginning )
        if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue == i)
        {
            //m_target->ApplyStatMod(Stats(i), m_modifier.m_amount,apply);
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER || ((Creature*)m_target)->isPet())
                m_target->ApplyStatBuffMod(Stats(i), m_modifier.m_amount, apply);
        }
    }
}

void Aura::HandleModPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
    }
}

void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    // Recalculate bonus
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleAuraModDispelResist(bool apply, bool Real)
{
    if(!Real || !apply)
        return;

    if(GetId() == 33206)
        m_target->CastSpell(m_target, 44416, true, NULL, this, GetCasterGUID());
}

void Aura::HandleModSpellDamagePercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    // Recalculate bonus
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;
    // implemented in Unit::SpellHealingBonus
    // this information is for client side only
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    //save current and max HP before applying aura
    uint32 curHPValue = m_target->GetHealth();
    uint32 maxHPValue = m_target->GetMaxHealth();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER || ((Creature*)m_target)->isPet())
                m_target->ApplyStatPercentBuffMod(Stats(i), m_modifier.m_amount, apply );
        }
    }

    //recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
    if ((m_modifier.m_miscvalue == STAT_STAMINA) && (maxHPValue > 0) && (m_spellProto->Attributes & 0x10))
    {
        // newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
        uint32 newHPValue = (m_target->GetMaxHealth() * curHPValue) / maxHPValue;
        m_target->SetHealth(newHPValue);
    }
}

void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(m_modifier.m_miscvalue != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
        return;
    }

    // Recalculate Armor
    m_target->UpdateArmor();
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void Aura::HandleAuraModTotalHealthPercentRegen(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandleAuraModTotalManaPercentRegen(bool apply, bool /*Real*/)
{
    if(m_modifier.periodictime == 0)
        m_modifier.periodictime = 1000;

    m_periodicTimer = m_modifier.periodictime;
    m_isPeriodic = apply;
}

void Aura::HandleModRegen(bool apply, bool /*Real*/)        // eating
{
    if(m_modifier.periodictime == 0)
        m_modifier.periodictime = 5000;

    m_periodicTimer = 5000;
    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegen(bool apply, bool Real)       // drinking
{
    if (!Real)
        return;

    Powers pt = m_target->getPowerType();
    if(m_modifier.periodictime == 0)
    {
        // Anger Management (only spell use this aura for rage)
        if (pt == POWER_RAGE)
            m_modifier.periodictime = 3000;
        else
            m_modifier.periodictime = 2000;
    }

    m_periodicTimer = 5000;

    if (m_target->GetTypeId() == TYPEID_PLAYER && m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)m_target)->UpdateManaRegen();

    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if (m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Update manaregen value
    if (m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)m_target)->UpdateManaRegen();
}

void Aura::HandleModManaRegen(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if (m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    //Note: an increase in regen does NOT cause threat.
    ((Player*)m_target)->UpdateManaRegen();
}

void Aura::HandleComprehendLanguage(bool apply, bool /*Real*/)
{
    if(apply)
        m_target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    else
        m_target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
}

void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
    // Special case with temporary increase max/current health
    switch(GetId())
    {
        case 12976:                                         // Warrior Last Stand triggered spell
        case 28726:                                         // Nightmare Seed ( Nightmare Seed )
        case 34511:                                         // Valor (Bulwark of Kings, Bulwark of the Ancient Kings)
        case 44055: case 55915: case 55917: case 67596:     // Tremendous Fortitude (Battlemaster's Alacrity)
        case 50322:                                         // Survival Instincts
        case 54443:                                         // Demonic Empowerment (Voidwalker)
        {
            if(Real)
            {
                if(apply)
                {
                    // Demonic Empowerment (Voidwalker) - special case, store percent in data
                    // recalculate to full amount at apply for proper remove
                    if (GetId() == 54443)
                        m_modifier.m_amount = m_target->GetMaxHealth() * m_modifier.m_amount / 100;

                    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                    m_target->ModifyHealth(m_modifier.m_amount);
                }
                else
                {
                    if (int32(m_target->GetHealth()) > m_modifier.m_amount)
                        m_target->ModifyHealth(-m_modifier.m_amount);
                    else
                        m_target->SetHealth(1);
                    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                }
            }
            return;
        }
    }

    // generic case
    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void  Aura::HandleAuraModIncreaseMaxHealth(bool apply, bool /*Real*/)
{
    uint32 oldhealth = m_target->GetHealth();
    double healthPercentage = (double)oldhealth / (double)m_target->GetMaxHealth();

    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);

    // refresh percentage
    if(oldhealth > 0)
    {
        uint32 newhealth = uint32(ceil((double)m_target->GetMaxHealth() * healthPercentage));
        if(newhealth==0)
            newhealth = 1;

        m_target->SetHealth(newhealth);
    }
}

void Aura::HandleAuraModIncreaseEnergy(bool apply, bool Real)
{
    Powers powerType = m_target->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    // Special case with temporary increase max/current power (percent)
    if (GetId()==64904)                                     // Hymn of Hope
    {
        if(Real)
        {
            uint32 val = m_target->GetPower(powerType);
            m_target->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
            m_target->SetPower(powerType, apply ? val*(100+m_modifier.m_amount)/100 : val*100/(100+m_modifier.m_amount));
        }
        return;
    }

    // generic flat case
    m_target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
    Powers powerType = m_target->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    m_target->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraIncreaseBaseHealthPercent(bool apply, bool /*Real*/)
{
    m_target->HandleStatModifier(UNIT_MOD_HEALTH, BASE_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateParryPercentage();
}

void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateDodgePercentage();
    //sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModBlockPercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateBlockPercentage();
    //sLog.outError("BONUS BLOCK CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateManaRegen();
}

void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // apply item specific bonuses for already equipped weapon
    if(Real)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)m_target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)m_target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
    }

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if (GetSpellProto()->EquippedItemClass == -1)
    {
        ((Player*)m_target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)m_target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)m_target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (m_modifier.m_amount), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)m_target)->UpdateMeleeHitChances();
        ((Player*)m_target)->UpdateRangedHitChances();
    }
    else
    {
        m_target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
        m_target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)m_target)->UpdateSpellHitChances();
    }
    else
    {
        m_target->m_modSpellHitChance += apply ? m_modifier.m_amount: (-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)m_target)->UpdateAllSpellCritChances();
    }
    else
    {
        m_target->m_baseSpellCritChance += apply ? m_modifier.m_amount:(-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellCritChanceShool(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    for(int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (m_modifier.m_miscvalue & (1<<school))
            ((Player*)m_target)->UpdateSpellCritChance(school);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
    m_target->ApplyCastTimePercentMod(m_modifier.m_amount,apply);
}

void Aura::HandleModMeleeRangedSpeedPct(bool apply, bool /*Real*/)
{
    m_target->ApplyAttackTimePercentMod(BASE_ATTACK, m_modifier.m_amount, apply);
    m_target->ApplyAttackTimePercentMod(OFF_ATTACK, m_modifier.m_amount, apply);
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK, m_modifier.m_amount, apply);
}

void Aura::HandleModCombatSpeedPct(bool apply, bool /*Real*/)
{
    m_target->ApplyCastTimePercentMod(m_modifier.m_amount, apply);
    m_target->ApplyAttackTimePercentMod(BASE_ATTACK, m_modifier.m_amount, apply);
    m_target->ApplyAttackTimePercentMod(OFF_ATTACK, m_modifier.m_amount, apply);
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK, m_modifier.m_amount, apply);
}

void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
    if(!m_target->isAlive() )
        return;

    m_target->ApplyAttackTimePercentMod(BASE_ATTACK,m_modifier.m_amount,apply);
}

void Aura::HandleHaste(bool apply, bool /*Real*/)
{
    m_target->ApplyAttackTimePercentMod(BASE_ATTACK, m_modifier.m_amount, apply);
    m_target->ApplyAttackTimePercentMod(OFF_ATTACK, m_modifier.m_amount, apply);
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK, m_modifier.m_amount, apply);
}

void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK, m_modifier.m_amount, apply);
}

void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK,m_modifier.m_amount, apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
    if((m_target->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
    //UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
    if((m_target->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    //UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerOfStatPercent(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // Recalculate bonus
    if(m_target->GetTypeId() == TYPEID_PLAYER && !(m_target->getClassMask() & CLASSMASK_WAND_USERS))
        ((Player*)m_target)->UpdateAttackPowerAndDamage(true);
}

void Aura::HandleAuraModAttackPowerOfStatPercent(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // Recalculate bonus
    if(m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->UpdateAttackPowerAndDamage(false);
}

void Aura::HandleAuraModAttackPowerOfArmor(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // Recalculate bonus
    if(m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->UpdateAttackPowerAndDamage(false);
}
/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
    // apply item specific bonuses for already equipped weapon
    if(Real && m_target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)m_target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)m_target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || m_target->GetTypeId() != TYPEID_PLAYER)
        {
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }

        if(m_target->GetTypeId() == TYPEID_PLAYER)
        {
            if(m_positive)
                m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
            else
                m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
        }
    }

    // Skip non magic case for speedup
    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if( GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0 )
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    if(m_target->GetTypeId() == TYPEID_PLAYER)
    {
        if(m_positive)
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
            }
        }
        else
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
            }
        }
        Pet* pet = m_target->GetPet();
        if(pet)
            pet->UpdateAttackPowerAndDamage();
    }
}

void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
    sLog.outDebug("AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);

    // apply item specific bonuses for already equipped weapon
    if(Real && m_target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)m_target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)m_target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wand
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || m_target->GetTypeId() != TYPEID_PLAYER)
        {
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
        // For show in client
        if(m_target->GetTypeId() == TYPEID_PLAYER)
            m_target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount/100.0f, apply);
    }

    // Skip non magic case for speedup
    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if( GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0 )
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage percent modifiers implemented in Unit::SpellDamageBonus
    // Send info to client
    if(m_target->GetTypeId() == TYPEID_PLAYER)
        for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            m_target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount/100.0f, apply);
}

void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    sLog.outDebug("AURA MOD OFFHAND DAMAGE");

    m_target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void Aura::HandleModPowerCostPCT(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    float amount = m_modifier.m_amount/100.0f;
    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if(m_modifier.m_miscvalue & (1<<i))
            m_target->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void Aura::HandleModPowerCost(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if(m_modifier.m_miscvalue & (1<<i))
            m_target->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
}

void Aura::HandleNoReagentUseAura(bool /*Apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;
    uint32 mask[3] = {0, 0, 0};
    Unit::AuraList const& noReagent = m_target->GetAurasByType(SPELL_AURA_NO_REAGENT_USE);
        for(Unit::AuraList::const_iterator i = noReagent.begin(); i !=  noReagent.end(); ++i)
        {
            uint32 const *ptr = (*i)->getAuraSpellClassMask();
            mask[0] |= ptr[0];
            mask[1] |= ptr[1];
            mask[2] |= ptr[2];
        }

    m_target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+0, mask[0]);
    m_target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+1, mask[1]);
    m_target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+2, mask[2]);
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void Aura::HandleShapeshiftBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 HotWSpellId = 0;
    uint32 MasterShaperSpellId = 0;

    uint32 form = GetModifier()->m_miscvalue;

    switch(form)
    {
        case FORM_CAT:
            spellId1 = 3025;
            HotWSpellId = 24900;
            MasterShaperSpellId = 48420;
            break;
        case FORM_TREE:
            spellId1 = 5420;
            spellId2 = 34123;
            MasterShaperSpellId = 48422;
            break;
        case FORM_TRAVEL:
            spellId1 = 5419;
            break;
        case FORM_AQUA:
            spellId1 = 5421;
            break;
        case FORM_BEAR:
            spellId1 = 1178;
            spellId2 = 21178;
            HotWSpellId = 24899;
            MasterShaperSpellId = 48418;
            break;
        case FORM_DIREBEAR:
            spellId1 = 9635;
            spellId2 = 21178;
            HotWSpellId = 24899;
            MasterShaperSpellId = 48418;
            break;
        case FORM_BATTLESTANCE:
            spellId1 = 21156;
            break;
        case FORM_DEFENSIVESTANCE:
            spellId1 = 7376;
            break;
        case FORM_BERSERKERSTANCE:
            spellId1 = 7381;
            break;
        case FORM_MOONKIN:
            spellId1 = 24905;
            MasterShaperSpellId = 48421;
            break;
        case FORM_FLIGHT:
            spellId1 = 33948;
            spellId2 = 34764;
            break;
        case FORM_FLIGHT_EPIC:
            spellId1 = 40122;
            spellId2 = 40121;
            break;
        case FORM_METAMORPHOSIS:
            spellId1 = 54817;
            spellId2 = 54879;
            break;
        case FORM_SPIRITOFREDEMPTION:
            spellId1 = 27792;
            spellId2 = 27795;                               // must be second, this important at aura remove to prevent to early iterator invalidation.
            break;
        case FORM_SHADOW:
            spellId1 = 49868;

            if(m_target->GetTypeId() == TYPEID_PLAYER)      // Spell 49868 have same category as main form spell and share cooldown
                ((Player*)m_target)->RemoveSpellCooldown(49868);
            break;
        case FORM_GHOSTWOLF:
            spellId1 = 67116;
            break;
        case FORM_AMBIENT:
        case FORM_GHOUL:
        case FORM_STEALTH:
        case FORM_CREATURECAT:
        case FORM_CREATUREBEAR:
            break;
    }

    if(apply)
    {
        if (spellId1)
            m_target->CastSpell(m_target, spellId1, true, NULL, this );
        if (spellId2)
            m_target->CastSpell(m_target, spellId2, true, NULL, this);

        if (m_target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = ((Player *)m_target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second->state == PLAYERSPELL_REMOVED) continue;
                if (itr->first==spellId1 || itr->first==spellId2) continue;
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !(spellInfo->Attributes & (SPELL_ATTR_PASSIVE | (1<<7))))
                    continue;
                if (spellInfo->Stances & (1<<(form-1)))
                    m_target->CastSpell(m_target, itr->first, true, NULL, this);
            }

            // Master Shapeshifter
            if (MasterShaperSpellId)
            {
                Unit::AuraList const& ShapeShifterAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = ShapeShifterAuras.begin(); i != ShapeShifterAuras.end(); i++)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 2851)
                    {
                        int32 ShiftMod = (*i)->GetModifier()->m_amount;
                        m_target->CastCustomSpell(m_target, MasterShaperSpellId, &ShiftMod, NULL, NULL, true);
                        break;
                    }
                }
            }

            // Leader of the Pack
            if (((Player*)m_target)->HasSpell(17007))
            {
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(24932);
                if (spellInfo && spellInfo->Stances & (1<<(form-1)))
                    m_target->CastSpell(m_target, 24932, true, NULL, this);
            }

            // Savage Roar
            if (form == FORM_CAT && ((Player*)m_target)->HasAura(52610))
                m_target->CastSpell(m_target, 62071, true);

            // Improved Moonkin Form
            if (form == FORM_MOONKIN)
            {
                Unit::AuraList const& dummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); i++)
                {
                    if ((*i)->GetSpellProto()->SpellFamilyName==SPELLFAMILY_DRUID &&
                        (*i)->GetSpellProto()->SpellIconID == 2855)
                    {
                        uint32 spell_id = 0;
                        switch((*i)->GetId())
                        {
                            case 48384:spell_id=50170;break;//Rank 1
                            case 48395:spell_id=50171;break;//Rank 2
                            case 48396:spell_id=50172;break;//Rank 3
                            default:
                                sLog.outError("Aura::HandleShapeshiftBoosts: Not handled rank of IMF (Spell: %u)",(*i)->GetId());
                                break;
                        }

                        if(spell_id)
                            m_target->CastSpell(m_target, spell_id, true, NULL, this);
                        break;
                    }
                }
            }

            // Heart of the Wild
            if (HotWSpellId)
            {
                Unit::AuraList const& mModTotalStatPct = m_target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for(Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
                    {
                        int32 HotWMod = (*i)->GetModifier()->m_amount;
                        if(GetModifier()->m_miscvalue == FORM_CAT)
                            HotWMod /= 2;

                        m_target->CastCustomSpell(m_target, HotWSpellId, &HotWMod, NULL, NULL, true, NULL, this);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if(spellId1)
            m_target->RemoveAurasDueToSpell(spellId1);
        if(spellId2)
            m_target->RemoveAurasDueToSpell(spellId2);
        if(MasterShaperSpellId)
            m_target->RemoveAurasDueToSpell(MasterShaperSpellId);

        Unit::AuraMap& tAuras = m_target->GetAuras();
        for (Unit::AuraMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->IsRemovedOnShapeLost())
            {
                m_target->RemoveAurasDueToSpell(itr->second->GetId());
                itr = tAuras.begin();
            }
            else
                ++itr;
        }
    }
}

void Aura::HandleSpellSpecificBoosts(bool apply)
{
    bool cast_at_remove = false;                            // if spell must be casted at last aura from stack remove
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 spellId3 = 0;
    uint32 spellId4 = 0;

    switch(GetSpellProto()->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            // Illusionary Barrier
            if(GetId() == 57350 && !apply && m_target->getPowerType() == POWER_MANA)
            {
                cast_at_remove = true;
                spellId1 = 60242;                           // Darkmoon Card: Illusion
            }
            else
                return;
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // Ice Barrier (non stacking from one caster)
            if (m_spellProto->SpellIconID == 32)
            {
                if (!apply && (m_removeMode == AURA_REMOVE_BY_DISPEL || (m_removeMode == AURA_REMOVE_BY_DEFAULT && !GetModifier()->m_amount)))
                {
                    Unit::AuraList const& dummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                    {
                        // Shattered Barrier
                        if ((*itr)->GetSpellProto()->SpellIconID == 2945)
                        {
                            cast_at_remove = true;
                            // first rank have 50% chance
                            if ((*itr)->GetId() != 44745 || roll_chance_i(50))
                                spellId1 = 55080;
                            break;
                        }
                    }
                }
                else
                    return;
            }
            else
                return;
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            if(!apply)
            {
                // Remove Blood Frenzy only if target no longer has any Deep Wound or Rend (applying is handled by procs)
                if (GetSpellProto()->Mechanic != MECHANIC_BLEED)
                    return;

                // If target still has one of Warrior's bleeds, do nothing
                Unit::AuraList const& PeriodicDamage = m_target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                for(Unit::AuraList::const_iterator i = PeriodicDamage.begin(); i != PeriodicDamage.end(); ++i)
                    if( (*i)->GetCasterGUID() == GetCasterGUID() &&
                        (*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARRIOR &&
                        (*i)->GetSpellProto()->Mechanic == MECHANIC_BLEED)
                        return;

                spellId1 = 30069;                           // Blood Frenzy (Rank 1)
                spellId2 = 30070;                           // Blood Frenzy (Rank 2)
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Fear (non stacking)
            if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000040000000000))
            {
                if(!apply)
                {
                    Unit* caster = GetCaster();
                    if(!caster)
                        return;

                    Unit::AuraList const& dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                    {
                        SpellEntry const* dummyEntry = (*itr)->GetSpellProto();
                        // Improved Fear
                        if (dummyEntry->SpellFamilyName == SPELLFAMILY_WARLOCK && dummyEntry->SpellIconID == 98)
                        {
                            cast_at_remove = true;
                            switch((*itr)->GetModifier()->m_amount)
                            {
                                // Rank 1
                                case 0: spellId1 = 60946; break;
                                // Rank 1
                                case 1: spellId1 = 60947; break;
                            }
                            break;
                        }
                    }
                }
                else
                    return;
            }
            else
                return;
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Shadow Word: Pain (need visual check fro skip improvement talent) or Vampiric Touch
            if (m_spellProto->SpellIconID == 234 && m_spellProto->SpellVisual[0] || m_spellProto->SpellIconID == 2213)
            {
                if (!apply && m_removeMode == AURA_REMOVE_BY_DISPEL)
                {
                    Unit* caster = GetCaster();
                    if(!caster)
                        return;

                    Unit::AuraList const& dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                    {
                        // Shadow Affinity
                        if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_PRIEST
                            && (*itr)->GetSpellProto()->SpellIconID == 178)
                        {
                            // custom cast code
                            int32 basepoints0 = (*itr)->GetModifier()->m_amount * caster->GetCreateMana() / 100;
                            caster->CastCustomSpell(caster, 64103, &basepoints0, NULL, NULL, true, NULL, this);
                            return;
                        }
                    }
                }
                else
                    return;
            }

            switch(GetId())
            {
                // Abolish Disease (remove 1 more poison effect with Body and Soul)
                case 552:
                {
                    if(apply)
                    {
                        int chance =0;
                        Unit::AuraList const& dummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                        for(Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                        {
                            SpellEntry const* dummyEntry = (*itr)->GetSpellProto();
                            // Body and Soul (talent ranks)
                            if (dummyEntry->SpellFamilyName == SPELLFAMILY_PRIEST && dummyEntry->SpellIconID == 2218 &&
                                dummyEntry->SpellVisual[0]==0)
                            {
                                chance = (*itr)->GetSpellProto()->CalculateSimpleValue(1);
                                break;
                            }
                        }

                        if(roll_chance_i(chance))
                            spellId1 = 64134;               // Body and Soul (periodic dispel effect)
                    }
                    else
                        spellId1 = 64134;                   // Body and Soul (periodic dispel effect)
                    break;
                }
                // Dispersion mana reg and immunity
                case 47585:
                    spellId1 = 60069;                       // Dispersion
                    spellId2 = 63230;                       // Dispersion
                    break;
                default:
                    return;
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
            // Sprint (skip non player casted spells by category)
            if (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000040) && GetSpellProto()->Category == 44)
            {
                if(!apply || m_target->HasAura(58039))      // Glyph of Blurred Speed
                    spellId1 = 61922;                       // Sprint (waterwalk)
                else
                    return;
            }
            else
                return;
            break;
        case SPELLFAMILY_HUNTER:
        {
            // The Beast Within and Bestial Wrath - immunity
            if (GetId() == 19574 || GetId() == 34471)
            {
                spellId1 = 24395;
                spellId2 = 24396;
                spellId3 = 24397;
                spellId4 = 26592;
            }
            // Aspect of the Dragonhawk dodge
            else if (GetSpellProto()->SpellFamilyFlags2 & 0x00001000)
            {
                spellId1 = 61848;

                // triggered spell have same category as main spell and cooldown
                if (apply && m_target->GetTypeId()==TYPEID_PLAYER)
                    ((Player*)m_target)->RemoveSpellCooldown(61848);
            }
            else
                return;
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            if (m_spellProto->Id == 31884)                  // Avenging Wrath
            {
                if(!apply)
                    spellId1 = 57318;                       // Sanctified Wrath (triggered)
                else
                {
                    int32 percent = 0;
                    Unit::AuraList const& dummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                    {
                        if ((*itr)->GetSpellProto()->SpellIconID == 3029)
                        {
                            percent = (*itr)->GetModifier()->m_amount;
                            break;
                        }
                    }

                    // apply in special way
                    if(percent)
                    {
                        spellId1 = 57318;                   // Sanctified Wrath (triggered)
                        // prevent aura deletion, specially in multi-boost case
                        SetInUse(true);
                        m_target->CastCustomSpell(m_target, spellId1, &percent, &percent, NULL, true, NULL, this);
                        SetInUse(false);
                    }
                    return;
                }
                break;
            }

            // Only process on player casting paladin aura
            // all aura bonuses applied also in aura area effect way to caster
            if (GetCasterGUID() != m_target->GetGUID() || !IS_PLAYER_GUID(GetCasterGUID()))
                return;

            if (GetSpellSpecific(m_spellProto->Id) != SPELL_AURA)
                return;

            // Sanctified Retribution and Swift Retribution (they share one aura), but not Retribution Aura (already gets modded)
            if ((GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000008))==0)
                spellId1 = 63531;                           // placeholder for talent spell mods
            // Improved Concentration Aura (auras bonus)
            spellId2 = 63510;                               // placeholder for talent spell mods
            // Improved Devotion Aura (auras bonus)
            spellId3 = 63514;                               // placeholder for talent spell mods
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            if (GetSpellSpecific(m_spellProto->Id) != SPELL_PRESENCE)
                return;

            // Frost Presence health
            if (GetId() == 48263)
                spellId1 = 61261;
            // Unholy Presence move speed
            else if (GetId() == 48265)
                spellId1 = 49772;
            else
                return;

            break;
        }
        default:
            return;
    }

    // prevent aura deletion, specially in multi-boost case
    SetInUse(true);

    if (apply || cast_at_remove)
    {
        if (spellId1)
            m_target->CastSpell(m_target, spellId1, true, NULL, this);
        if (spellId2 && !IsDeleted())
            m_target->CastSpell(m_target, spellId2, true, NULL, this);
        if (spellId3 && !IsDeleted())
            m_target->CastSpell(m_target, spellId3, true, NULL, this);
        if (spellId4 && !IsDeleted())
            m_target->CastSpell(m_target, spellId4, true, NULL, this);
    }
    else
    {
        if (spellId1)
            m_target->RemoveAurasByCasterSpell(spellId1, GetCasterGUID());
        if (spellId2)
            m_target->RemoveAurasByCasterSpell(spellId2, GetCasterGUID());
        if (spellId3)
            m_target->RemoveAurasByCasterSpell(spellId3, GetCasterGUID());
        if (spellId4)
            m_target->RemoveAurasByCasterSpell(spellId4, GetCasterGUID());
    }

    SetInUse(false);
}

void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_UNIT)
        return;

    CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(m_target->GetEntry());
    if(ci && ci->type == CREATURE_TYPE_BEAST)
        m_target->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
    if(apply)
        m_target->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    else
        m_target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
}

void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(apply)
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
    HandleAuraModPacify(apply, Real);
    HandleAuraModSilence(apply, Real);
}

void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(apply)
    {
        m_target->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
    else
    {
        m_target->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
}

void Aura::HandleAuraAllowFlight(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    // allow fly
    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
    else
        data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
    data.append(m_target->GetPackGUID());
    data << uint32(0);                                      // unk
    m_target->SendMessageToSet(&data, true);
}

void Aura::HandleModRating(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (m_modifier.m_miscvalue & (1 << rating))
            ((Player*)m_target)->ApplyRatingMod(CombatRating(rating), m_modifier.m_amount, apply);
}

void Aura::HandleModRatingFromStat(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;
    // Just recalculate ratings
    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (m_modifier.m_miscvalue & (1 << rating))
            ((Player*)m_target)->ApplyRatingMod(CombatRating(rating), 0, apply);
}

void Aura::HandleForceMoveForward(bool apply, bool Real)
{
    if(!Real || m_target->GetTypeId() != TYPEID_PLAYER)
        return;
    if(apply)
        m_target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
    else
        m_target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
}

void Aura::HandleAuraModExpertise(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateExpertise(BASE_ATTACK);
    ((Player*)m_target)->UpdateExpertise(OFF_ATTACK);
}

void Aura::HandleModTargetResistance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;
    // applied to damage as HandleNoImmediateEffect in Unit::CalcAbsorbResist and Unit::CalcArmorReducedDamage

    // show armor penetration
    if (m_target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
        m_target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, m_modifier.m_amount, apply);

    // show as spell penetration only full spell penetration bonuses (all resistances except armor and holy
    if (m_target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_SPELL)==SPELL_SCHOOL_MASK_SPELL)
        m_target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, m_modifier.m_amount, apply);
}

void Aura::HandleShieldBlockValue(bool apply, bool /*Real*/)
{
    BaseModType modType = FLAT_MOD;
    if(m_modifier.m_auraname == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if(m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *target = (Player*)m_target;

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if( !apply && m_duration==0 && target->GetComboTarget())
        if(Unit* unit = ObjectAccessor::GetUnit(*m_target,target->GetComboTarget()))
            target->AddComboPoints(unit, -m_modifier.m_amount);
}

void Aura::HandleModUnattackable( bool Apply, bool Real )
{
    if(Real && Apply)
     {
        m_target->CombatStop();
        m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
     }
    m_target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE,Apply);
}

void Aura::HandleSpiritOfRedemption( bool apply, bool Real )
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // prepare spirit state
    if(apply)
    {
        if(m_target->GetTypeId()==TYPEID_PLAYER)
        {
            // disable breath/etc timers
            ((Player*)m_target)->StopMirrorTimers();

            // set stand state (expected in this form)
            if(!m_target->IsStandState())
                m_target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        m_target->SetHealth(1);
    }
    // die at aura end
    else
        m_target->DealDamage(m_target, m_target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
}

void Aura::CleanupTriggeredSpells()
{
    uint32 tSpellId = m_spellProto->EffectTriggerSpell[GetEffIndex()];
    if(!tSpellId)
        return;

    SpellEntry const* tProto = sSpellStore.LookupEntry(tSpellId);
    if(!tProto)
        return;

    if(GetSpellDuration(tProto) != -1)
        return;

    // needed for spell 43680, maybe others
    // TODO: is there a spell flag, which can solve this in a more sophisticated way?
    if(m_spellProto->EffectApplyAuraName[GetEffIndex()] == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
            GetSpellDuration(m_spellProto) == m_spellProto->EffectAmplitude[GetEffIndex()])
        return;
    m_target->RemoveAurasDueToSpell(tSpellId);
}

void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    if (apply)
    {
        // prevent double apply bonuses
        if (m_target->GetTypeId()!=TYPEID_PLAYER || !((Player*)m_target)->GetSession()->PlayerLoading())
        {
            float DoneActualBenefit = 0.0f;
            switch(m_spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_PRIEST:
                    // Power Word: Shield
                    if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001))
                        //+80.68% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseHealingBonus(GetSpellSchoolMask(m_spellProto)) * 0.8068f;
                    break;
                case SPELLFAMILY_MAGE:
                    // Frost Ward, Fire Ward
                    if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000108))
                        //+10% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.1f;
                    // Ice Barrier
                    else if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000100000000))
                        //+80.67% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.8067f;
                    break;
                case SPELLFAMILY_WARLOCK:
                    // Shadow Ward
                    if (m_spellProto->SpellFamilyFlags2 & 0x00000040)
                        //+30% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.30f;
                    break;
                case SPELLFAMILY_PALADIN:
                    // Sacred Shield
                    // (check not strictly needed, only Sacred Shield has SPELL_AURA_SCHOOL_ABSORB in SPELLFAMILY_PALADIN at this time)
                    if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0008000000000000))
                    {
                        // +75% from spell power
                        DoneActualBenefit = caster->SpellBaseHealingBonus(GetSpellSchoolMask(m_spellProto)) * 0.75f;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
    else
    {
        if (caster &&
            // Power Word: Shield
            m_spellProto->SpellFamilyName == SPELLFAMILY_PRIEST && m_spellProto->Mechanic == MECHANIC_SHIELD &&
            (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001)) &&
            // completely absorbed or dispelled
            ((m_removeMode == AURA_REMOVE_BY_DEFAULT && !m_modifier.m_amount) || m_removeMode == AURA_REMOVE_BY_DISPEL))
        {
            Unit::AuraList const& vDummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
            for(Unit::AuraList::const_iterator itr = vDummyAuras.begin(); itr != vDummyAuras.end(); ++itr)
            {
                SpellEntry const* vSpell = (*itr)->GetSpellProto();

                // Rapture (main spell)
                if(vSpell->SpellFamilyName == SPELLFAMILY_PRIEST && vSpell->SpellIconID == 2894 && vSpell->Effect[1])
                {
                    switch((*itr)->GetEffIndex())
                    {
                        case 0:
                        {
                            // energize caster
                            int32 manapct1000 = 5 * ((*itr)->GetModifier()->m_amount + sSpellMgr.GetSpellRank(vSpell->Id));
                            int32 basepoints0 = caster->GetMaxPower(POWER_MANA) * manapct1000 / 1000;
                            caster->CastCustomSpell(caster, 47755, &basepoints0, NULL, NULL, true);
                            break;
                        }
                        case 1:
                        {
                            // energize target
                            if (!roll_chance_i((*itr)->GetModifier()->m_amount) || caster->HasAura(63853))
                                break;

                            switch(m_target->getPowerType())
                            {
                            case POWER_RUNIC_POWER:
                                m_target->CastSpell(m_target, 63652, true, NULL, NULL, m_caster_guid);
                                break;
                            case POWER_RAGE:
                                m_target->CastSpell(m_target, 63653, true, NULL, NULL, m_caster_guid);
                                break;
                            case POWER_MANA:
                                {
                                    int32 basepoints0 = m_target->GetMaxPower(POWER_MANA) * 2 / 100;
                                    m_target->CastCustomSpell(m_target, 63654, &basepoints0, NULL, NULL, true);
                                    break;
                                }
                            case POWER_ENERGY:
                                m_target->CastSpell(m_target, 63655, true, NULL, NULL, m_caster_guid);
                                break;
                            default:
                                break;
                            }

                            //cooldwon aura
                            caster->CastSpell(caster, 63853, true);
                            break;
                        }
                        default:
                            sLog.outError("Changes in R-dummy spell???: effect 3");
                            break;
                    }
                }
            }
        }
    }
}

void Aura::PeriodicTick()
{
    switch(m_modifier.m_auraname)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(m_target, GetSpellProto(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            // some auras remove at specific health level or more
            if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
            {
                switch(GetId())
                {
                    case 43093: case 31956: case 38801:
                    case 35321: case 38363: case 39215:
                        if(m_target->GetHealth() == m_target->GetMaxHealth() )
                        {
                            m_target->RemoveAurasDueToSpell(GetId());
                            return;
                        }
                        break;
                    case 38772:
                    {
                        uint32 percent =
                            GetEffIndex() < 2 && GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_DUMMY ?
                            pCaster->CalculateSpellDamage(GetSpellProto(), GetEffIndex() + 1, GetSpellProto()->EffectBasePoints[GetEffIndex() + 1], m_target) :
                            100;
                        if(m_target->GetHealth() * 100 >= m_target->GetMaxHealth() * percent )
                        {
                            m_target->RemoveAurasDueToSpell(GetId());
                            return;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            uint32 absorb = 0;
            uint32 resist = 0;
            CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage;

            if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
            {
                pdamage = amount;

                // SpellDamageBonus for magic spells
                if(GetSpellProto()->DmgClass == SPELL_DAMAGE_CLASS_NONE || GetSpellProto()->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
                    pdamage = pCaster->SpellDamageBonus(m_target, GetSpellProto(), pdamage, DOT, GetStackAmount());
                // MeleeDamagebonus for weapon based spells
                else
                {
                    WeaponAttackType attackType = GetWeaponAttackType(GetSpellProto());
                    pdamage = pCaster->MeleeDamageBonus(m_target, pdamage, attackType, GetSpellProto(), DOT, GetStackAmount());
                }

                // Calculate armor mitigation if it is a physical spell
                // But not for bleed mechanic spells
                if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_NORMAL &&
                    GetEffectMechanic(GetSpellProto(), m_effIndex) != MECHANIC_BLEED)
                {
                    uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(m_target, pdamage);
                    cleanDamage.damage += pdamage - pdamageReductedArmor;
                    pdamage = pdamageReductedArmor;
                }

                // Curse of Agony damage-per-tick calculation
                if (GetSpellProto()->SpellFamilyName==SPELLFAMILY_WARLOCK && (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000400)) && GetSpellProto()->SpellIconID==544)
                {
                    // 1..4 ticks, 1/2 from normal tick damage
                    if (m_duration >= ((m_maxduration-m_modifier.periodictime) * 2 / 3))
                        pdamage = pdamage/2;
                    // 9..12 ticks, 3/2 from normal tick damage
                    else if(m_duration < ((m_maxduration-m_modifier.periodictime) / 3))
                        pdamage += (pdamage + 1) / 2;       // +1 prevent 0.5 damage possible lost at 1..4 ticks
                    // 5..8 ticks have normal tick damage
                }
            }
            else
                pdamage = uint32(m_target->GetMaxHealth()*amount/100);

            // This method can modify pdamage
            bool isCrit = IsCritFromAbilityAura(pCaster, pdamage);

            // send critical in hit info for threat calculation
            if (isCrit)
            {
                cleanDamage.hitOutCome = MELEE_HIT_CRIT;
                // Resilience - reduce crit damage
                pdamage -= m_target->GetSpellCritDamageReduction(pdamage);
            }

            // only from players
            // FIXME: need use SpellDamageBonus instead?
            if (IS_PLAYER_GUID(m_caster_guid))
                pdamage -= m_target->GetSpellDamageReduction(pdamage);

            pCaster->CalcAbsorbResist(m_target, GetSpellSchoolMask(GetSpellProto()), DOT, pdamage, &absorb, &resist, !(GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) attacked %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId(),absorb);

            pCaster->DealDamageMods(m_target, pdamage, &absorb);

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, absorb, resist, 0.0f, isCrit);
            m_target->SendPeriodicAuraLog(&pInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
            pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);
            if (pdamage)
                procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;
            pCaster->ProcDamageAndSpell(m_target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, GetSpellProto());

            pCaster->DealDamage(m_target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), true);
            break;
        }
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if(!pCaster->isAlive())
                return;

            if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(m_target, GetSpellProto(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            uint32 absorb=0;
            uint32 resist=0;
            CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );

            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            //Calculate armor mitigation if it is a physical spell
            if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_NORMAL)
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(m_target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            pdamage = pCaster->SpellDamageBonus(m_target, GetSpellProto(), pdamage, DOT, GetStackAmount());

            pCaster->CalcAbsorbResist(m_target, GetSpellSchoolMask(GetSpellProto()), DOT, pdamage, &absorb, &resist, !(GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));

            if(m_target->GetHealth() < pdamage)
                pdamage = uint32(m_target->GetHealth());

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) health leech of %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId(),absorb);

            pCaster->SendSpellNonMeleeDamageLog(m_target, GetId(), pdamage, GetSpellSchoolMask(GetSpellProto()), absorb, resist, false, 0);

            float multiplier = GetSpellProto()->EffectMultipleValue[GetEffIndex()] > 0 ? GetSpellProto()->EffectMultipleValue[GetEffIndex()] : 1;

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
            pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage-absorb-resist);
            if (pdamage)
                procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;
            pCaster->ProcDamageAndSpell(m_target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, GetSpellProto());
            int32 new_damage = pCaster->DealDamage(m_target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), false);

            if (!m_target->isAlive() && pCaster->IsNonMeleeSpellCasted(false))
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                    if (Spell* spell = pCaster->GetCurrentSpell(CurrentSpellTypes(i)))
                        if (spell->m_spellInfo->Id == GetId())
                            spell->cancel();


            if(Player *modOwner = pCaster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, multiplier);

            uint32 heal = pCaster->SpellHealingBonus(pCaster, GetSpellProto(), uint32(new_damage * multiplier), DOT, GetStackAmount());

            int32 gain = pCaster->DealHeal(pCaster, heal, GetSpellProto());
            pCaster->getHostileRefManager().threatAssist(pCaster, gain * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            // heal for caster damage (must be alive)
            if(m_target != pCaster && GetSpellProto()->SpellVisual[0] == 163 && !pCaster->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage;

            if(m_modifier.m_auraname==SPELL_AURA_OBS_MOD_HEALTH)
                pdamage = uint32(m_target->GetMaxHealth() * amount / 100);
            else
            {
                pdamage = amount;

                // Wild Growth (1/7 - 6 + 2*ramainTicks) %
                if (m_spellProto->SpellFamilyName == SPELLFAMILY_DRUID && m_spellProto->SpellIconID == 2864)
                {
                    int32 ticks = m_maxduration/m_modifier.periodictime;
                    int32 remainingTicks = int32(float(m_duration) / m_modifier.periodictime + 0.5);
                    pdamage = int32(pdamage) + int32(amount)*ticks*(-6+2*remainingTicks)/100;
                }
            }

            pdamage = pCaster->SpellHealingBonus(m_target, GetSpellProto(), pdamage, DOT, GetStackAmount());

            // This method can modify pdamage
            bool isCrit = IsCritFromAbilityAura(pCaster, pdamage);

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) heal of %u (TypeId: %u) for %u health inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0, 0.0f, isCrit);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain = m_target->ModifyHealth(pdamage);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;
            pCaster->ProcDamageAndSpell(m_target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, gain, BASE_ATTACK, m_spellProto);

            // add HoTs to amount healed in bgs
            if( pCaster->GetTypeId() == TYPEID_PLAYER )
                if( BattleGround *bg = ((Player*)pCaster)->GetBattleGround() )
                    bg->UpdatePlayerScore(((Player*)pCaster), SCORE_HEALING_DONE, gain);

            m_target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, GetSpellProto());

            SpellEntry const* spellProto = GetSpellProto();

            // heal for caster damage
            if(m_target != pCaster && spellProto->SpellVisual[0] == 163)
            {
                uint32 dmg = spellProto->manaPerSecond;
                if(pCaster->GetHealth() <= dmg && pCaster->GetTypeId()==TYPEID_PLAYER)
                {
                    pCaster->RemoveAurasDueToSpell(GetId());

                    // finish current generic/channeling spells, don't affect autorepeat
                    pCaster->FinishSpell(CURRENT_GENERIC_SPELL);
                    pCaster->FinishSpell(CURRENT_CHANNELED_SPELL);
                }
                else
                {
                    uint32 damage = gain;
                    uint32 absorb = 0;
                    pCaster->DealDamageMods(pCaster, damage, &absorb);
                    pCaster->SendSpellNonMeleeDamageLog(pCaster, GetId(), damage, GetSpellSchoolMask(GetSpellProto()), absorb, 0, false, 0, false);

                    CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );
                    pCaster->DealDamage(pCaster, damage, &cleanDamage, NODAMAGE, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), true);
                }
            }

//            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;//   | PROC_FLAG_SUCCESSFUL_HEAL;
//            uint32 procVictim   = 0;//ROC_FLAG_ON_TAKE_PERIODIC | PROC_FLAG_TAKEN_HEAL;
            // ignore item heals
//            if(procSpell && !haveCastItem)
//                pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);
            break;
        }
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        {
            if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                return;

            Powers power = Powers(m_modifier.m_miscvalue);

            // power type might have changed between aura applying and tick (druid's shapeshift)
            if(m_target->getPowerType() != power)
                return;

            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if(!pCaster->isAlive())
                return;

            if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(m_target, GetSpellProto(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            // Special case: draining x% of mana (up to a maximum of 2*x% of the caster's maximum mana)
            // It's mana percent cost spells, m_modifier.m_amount is percent drain from target
            if (m_spellProto->ManaCostPercentage)
            {
                // max value
                uint32 maxmana = pCaster->GetMaxPower(power)  * pdamage * 2 / 100;
                pdamage = m_target->GetMaxPower(power) * pdamage / 100;
                if(pdamage > maxmana)
                    pdamage = maxmana;
            }

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) power leech of %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            int32 drain_amount = m_target->GetPower(power) > pdamage ? pdamage : m_target->GetPower(power);

            // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
            if (power == POWER_MANA)
                drain_amount -= m_target->GetSpellCritDamageReduction(drain_amount);

            m_target->ModifyPower(power, -drain_amount);

            float gain_multiplier = 0;

            if(pCaster->GetMaxPower(power) > 0)
            {
                gain_multiplier = GetSpellProto()->EffectMultipleValue[GetEffIndex()];

                if(Player *modOwner = pCaster->GetSpellModOwner())
                    modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, gain_multiplier);
            }

            SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, 0, gain_multiplier);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain_amount = int32(drain_amount * gain_multiplier);

            if(gain_amount)
            {
                int32 gain = pCaster->ModifyPower(power, gain_amount);
                m_target->AddThreat(pCaster, float(gain) * 0.5f, pInfo.critical, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
            }
            break;
        }
        case SPELL_AURA_PERIODIC_ENERGIZE:
        {
            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                break;

            Powers power = Powers(m_modifier.m_miscvalue);

            if(m_target->GetMaxPower(power) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0, 0.0f);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain = m_target->ModifyPower(power,pdamage);

            if(Unit* pCaster = GetCaster())
                m_target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_OBS_MOD_MANA:
        {
            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage = uint32(m_target->GetMaxPower(POWER_MANA) * amount / 100);

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u mana inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            if(m_target->GetMaxPower(POWER_MANA) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0, 0.0f);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain = m_target->ModifyPower(POWER_MANA, pdamage);

            if(Unit* pCaster = GetCaster())
                m_target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_POWER_BURN_MANA:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            // Check for immune (not use charges)
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            int32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            Powers powerType = Powers(m_modifier.m_miscvalue);

            if(!m_target->isAlive() || m_target->getPowerType() != powerType)
                return;

            // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
            if (powerType == POWER_MANA)
                pdamage -= m_target->GetSpellCritDamageReduction(pdamage);

            uint32 gain = uint32(-m_target->ModifyPower(powerType, -pdamage));

            gain = uint32(gain * GetSpellProto()->EffectMultipleValue[GetEffIndex()]);

            SpellEntry const* spellProto = GetSpellProto();
            // maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
            SpellNonMeleeDamage damageInfo(pCaster, m_target, spellProto->Id, spellProto->SchoolMask);
            pCaster->CalculateSpellDamage(&damageInfo, gain, spellProto);

            pCaster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

            pCaster->SendSpellNonMeleeDamageLog(&damageInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
            uint32 procEx       = createProcExtendMask(&damageInfo, SPELL_MISS_NONE);
            if (damageInfo.damage)
                procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;

            pCaster->ProcDamageAndSpell(damageInfo.target, procAttacker, procVictim, procEx, damageInfo.damage, BASE_ATTACK, spellProto);

            pCaster->DealSpellDamage(&damageInfo, true);
            break;
        }
        case SPELL_AURA_MOD_REGEN:
        {
            int32 gain = m_target->ModifyHealth(m_modifier.m_amount);
            if (Unit *caster = GetCaster())
                m_target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_MOD_POWER_REGEN:
        {
            Powers pt = m_target->getPowerType();
            if(int32(pt) != m_modifier.m_miscvalue)
                return;

            if ( GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED )
            {
                // eating anim
                m_target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
            }
            else if( GetId() == 20577 )
            {
                // cannibalize anim
                m_target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
            }

            // Anger Management
            // amount = 1+ 16 = 17 = 3,4*5 = 10,2*5/3
            // so 17 is rounded amount for 5 sec tick grow ~ 1 range grow in 3 sec
            if(pt == POWER_RAGE)
                m_target->ModifyPower(pt, m_modifier.m_amount * 3 / 5);
            break;
        }
        // Here tick dummy auras
        case SPELL_AURA_DUMMY:                              // some spells have dummy aura
        case SPELL_AURA_PERIODIC_DUMMY:
        {
            PeriodicDummyTick();
            break;
        }
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
        {
            TriggerSpell();
            break;
        }
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
        {
            TriggerSpellWithValue();
            break;
        }
        default:
            break;
    }
}

void Aura::PeriodicDummyTick()
{
    SpellEntry const* spell = GetSpellProto();
    switch (spell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch (spell->Id)
            {
                // Drink
                case 430:
                case 431:
                case 432:
                case 1133:
                case 1135:
                case 1137:
                case 10250:
                case 22734:
                case 27089:
                case 34291:
                case 43182:
                case 43183:
                case 43706:
                case 46755:
                case 49472: // Drink Coffee
                case 57073:
                case 61830:
                {
                    if (m_target->GetTypeId() != TYPEID_PLAYER)
                        return;
                    // Search SPELL_AURA_MOD_POWER_REGEN aura for this spell and add bonus
                    Unit::AuraList const& aura = m_target->GetAurasByType(SPELL_AURA_MOD_POWER_REGEN);
                    for(Unit::AuraList::const_iterator i = aura.begin(); i != aura.end(); ++i)
                    {
                        if ((*i)->GetId() == GetId())
                        {
                            (*i)->GetModifier()->m_amount = m_modifier.m_amount;
                            ((Player*)m_target)->UpdateManaRegen();
                            // Disable continue
                            m_isPeriodic = false;
                            return;
                        }
                    }
                    return;
                }
                // Forsaken Skills
                case 7054:
                {
                    // Possibly need cast one of them (but
                    // 7038 Forsaken Skill: Swords
                    // 7039 Forsaken Skill: Axes
                    // 7040 Forsaken Skill: Daggers
                    // 7041 Forsaken Skill: Maces
                    // 7042 Forsaken Skill: Staves
                    // 7043 Forsaken Skill: Bows
                    // 7044 Forsaken Skill: Guns
                    // 7045 Forsaken Skill: 2H Axes
                    // 7046 Forsaken Skill: 2H Maces
                    // 7047 Forsaken Skill: 2H Swords
                    // 7048 Forsaken Skill: Defense
                    // 7049 Forsaken Skill: Fire
                    // 7050 Forsaken Skill: Frost
                    // 7051 Forsaken Skill: Holy
                    // 7053 Forsaken Skill: Shadow
                    return;
                }
                case 7057:                                  // Haunting Spirits
                    if (roll_chance_i(33))
                        m_target->CastSpell(m_target,m_modifier.m_amount,true,NULL,this);
                    return;
//              // Panda
//              case 19230: break;
//              // Gossip NPC Periodic - Talk
//              case 33208: break;
//              // Gossip NPC Periodic - Despawn
//              case 33209: break;
//              // Steal Weapon
//              case 36207: break;
//              // Simon Game START timer, (DND)
//              case 39993: break;
//              // Knockdown Fel Cannon: break; The Aggro Burst
//              case 40119: break;
//              // Old Mount Spell
//              case 40154: break;
//              // Magnetic Pull
//              case 40581: break;
//              // Ethereal Ring: break; The Bolt Burst
//              case 40801: break;
//              // Crystal Prison
//              case 40846: break;
//              // Copy Weapon
//              case 41054: break;
//              // Dementia
//              case 41404: break;
//              // Ethereal Ring Visual, Lightning Aura
//              case 41477: break;
//              // Ethereal Ring Visual, Lightning Aura (Fork)
//              case 41525: break;
//              // Ethereal Ring Visual, Lightning Jumper Aura
//              case 41567: break;
//              // No Man's Land
//              case 41955: break;
//              // Headless Horseman - Fire
//              case 42074: break;
//              // Headless Horseman - Visual - Large Fire
//              case 42075: break;
//              // Headless Horseman - Start Fire, Periodic Aura
//              case 42140: break;
//              // Ram Speed Boost
//              case 42152: break;
//              // Headless Horseman - Fires Out Victory Aura
//              case 42235: break;
//              // Pumpkin Life Cycle
//              case 42280: break;
//              // Brewfest Request Chick Chuck Mug Aura
//              case 42537: break;
//              // Squashling
//              case 42596: break;
//              // Headless Horseman Climax, Head: Periodic
//              case 42603: break;
//              // Fire Bomb
//              case 42621: break;
//              // Headless Horseman - Conflagrate, Periodic Aura
//              case 42637: break;
//              // Headless Horseman - Create Pumpkin Treats Aura
//              case 42774: break;
//              // Headless Horseman Climax - Summoning Rhyme Aura
//              case 42879: break;
//              // Tricky Treat
//              case 42919: break;
//              // Giddyup!
//              case 42924: break;
//              // Ram - Trot
//              case 42992: break;
//              // Ram - Canter
//              case 42993: break;
//              // Ram - Gallop
//              case 42994: break;
//              // Ram Level - Neutral
//              case 43310: break;
//              // Headless Horseman - Maniacal Laugh, Maniacal, Delayed 17
//              case 43884: break;
//              // Wretched!
//              case 43963: break;
//              // Headless Horseman - Maniacal Laugh, Maniacal, other, Delayed 17
//              case 44000: break;
//              // Energy Feedback
//              case 44328: break;
//              // Romantic Picnic
//              case 45102: break;
//              // Romantic Picnic
//              case 45123: break;
//              // Looking for Love
//              case 45124: break;
//              // Kite - Lightning Strike Kite Aura
//              case 45197: break;
//              // Rocket Chicken
//              case 45202: break;
//              // Copy Offhand Weapon
//              case 45205: break;
//              // Upper Deck - Kite - Lightning Periodic Aura
//              case 45207: break;
//              // Kite -Sky  Lightning Strike Kite Aura
//              case 45251: break;
//              // Ribbon Pole Dancer Check Aura
//              case 45390: break;
//              // Holiday - Midsummer, Ribbon Pole Periodic Visual
//              case 45406: break;
//              // Parachute
//              case 45472: break;
//              // Alliance Flag, Extra Damage Debuff
//              case 45898: break;
//              // Horde Flag, Extra Damage Debuff
//              case 45899: break;
//              // Ahune - Summoning Rhyme Aura
//              case 45926: break;
//              // Ahune - Slippery Floor
//              case 45945: break;
//              // Ahune's Shield
//              case 45954: break;
//              // Nether Vapor Lightning
//              case 45960: break;
//              // Darkness
//              case 45996: break;
//              // Summon Blood Elves Periodic
//              case 46041: break;
//              // Transform Visual Missile Periodic
//              case 46205: break;
//              // Find Opening Beam End
//              case 46333: break;
//              // Ice Spear Control Aura
//              case 46371: break;
//              // Hailstone Chill
//              case 46458: break;
//              // Hailstone Chill, Internal
//              case 46465: break;
//              // Chill, Internal Shifter
//              case 46549: break;
//              // Summon Ice Spear Knockback Delayer
//              case 46878: break;
//              // Burninate Effect
//              case 47214: break;
//              // Fizzcrank Practice Parachute
//              case 47228: break;
//              // Send Mug Control Aura
//              case 47369: break;
//              // Direbrew's Disarm (precast)
//              case 47407: break;
//              // Mole Machine Port Schedule
//              case 47489: break;
//              case 47941: break; // Crystal Spike
//              case 48200: break; // Healer Aura
//              case 48630: break; // Summon Gauntlet Mobs Periodic
//              case 49313: break; // Proximity Mine Area Aura
//              // Mole Machine Portal Schedule
//              case 49466: break;
//              case 49555: break; // Corpse Explode
//              case 49592: break; // Temporal Rift
//              case 49957: break; // Cutting Laser
//              case 50085: break; // Slow Fall
//              // Listening to Music
//              case 50493: break;
//              // Love Rocket Barrage
//              case 50530: break;
// Exist more after, need add later
                default:
                    break;
            }
            // Prey on the Weak
            if (spell->SpellIconID == 2983)
            {
                Unit *target=m_target->getVictim();
                if (target && (m_target->GetHealth() * 100 / m_target->GetMaxHealth() > target->GetHealth() * 100 / target->GetMaxHealth()))
                {
                    if(!m_target->HasAura(58670))
                    {
                        int32 basepoints = GetBasePoints();
                        m_target->CastCustomSpell(m_target, 58670, &basepoints, 0, 0, true);
                    }
                }
                else
                    m_target->RemoveAurasDueToSpell(58670);
            }
            break;
        case SPELLFAMILY_MAGE:
        {
            // Mirror Image
//            if (spell->Id == 55342)
//                return;
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch (spell->Id)
            {
                // Frenzied Regeneration
                case 22842:
                {
                    // Converts up to 10 rage per second into health for $d.  Each point of rage is converted into ${$m2/10}.1% of max health.
                    // Should be manauser
                    if (m_target->getPowerType() != POWER_RAGE)
                        return;
                    uint32 rage = m_target->GetPower(POWER_RAGE);
                    // Nothing todo
                    if (rage == 0)
                        return;
                    int32 mod = (rage < 100) ? rage : 100;
                    int32 points = m_target->CalculateSpellDamage(spell, 1, spell->EffectBasePoints[1], m_target);
                    int32 regen = m_target->GetMaxHealth() * (mod * points / 10) / 1000;
                    m_target->CastCustomSpell(m_target, 22845, &regen, NULL, NULL, true, NULL, this);
                    m_target->SetPower(POWER_RAGE, rage-mod);
                    return;
                }
                // Force of Nature
                case 33831:
                    return;
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch (spell->Id)
            {
                // Killing Spree
                case 51690:
                {
                    if (m_target->hasUnitState(UNIT_STAT_STUNNED) || m_target->HasAuraType(SPELL_AURA_MOD_FEAR))
                        return;

                    std::list<Unit*> targets;
                    {
                        // eff_radius ==0
                        float radius = GetSpellMaxRange(sSpellRangeStore.LookupEntry(spell->rangeIndex));

                        CellPair p(MaNGOS::ComputeCellPair(m_target->GetPositionX(),m_target->GetPositionY()));
                        Cell cell(p);
                        cell.data.Part.reserved = ALL_DISTRICT;

                        MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck u_check(m_target, m_target, radius);
                        MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck> checker(m_target, targets, u_check);

                        TypeContainerVisitor<MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck>, GridTypeMapContainer > grid_object_checker(checker);
                        TypeContainerVisitor<MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck>, WorldTypeMapContainer > world_object_checker(checker);

                        CellLock<GridReadGuard> cell_lock(cell, p);

                        cell_lock->Visit(cell_lock, grid_object_checker,  *m_target->GetMap(), *m_target, radius);
                        cell_lock->Visit(cell_lock, world_object_checker, *m_target->GetMap(), *m_target, radius);
                    }

                    if(targets.empty())
                        return;

                    std::list<Unit*>::const_iterator itr = targets.begin();
                    std::advance(itr, rand()%targets.size());
                    Unit* target = *itr;

                    m_target->CastSpell(target, 57840, true);
                    m_target->CastSpell(target, 57841, true);
                    return;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Explosive Shot
            if (spell->SpellFamilyFlags & UI64LIT(0x8000000000000000))
            {
                m_target->CastCustomSpell(m_target, 53352, &m_modifier.m_amount, 0, 0, true, 0, this, GetCasterGUID());
                return;
            }
            switch (spell->Id)
            {
                // Harpooner's Mark
                // case 40084:
                //    return;
                // Feeding Frenzy Rank 1
                case 53511:
                    if ( m_target->GetHealth() * 100 < m_target->GetMaxHealth() * 35 )
                        m_target->CastSpell(m_target, 60096, true, 0, this);
                    return;
                // Feeding Frenzy Rank 2
                case 53512:
                    if ( m_target->GetHealth() * 100 < m_target->GetMaxHealth() * 35 )
                        m_target->CastSpell(m_target, 60097, true, 0, this);
                    return;
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Astral Shift
            if (spell->Id == 52179)
            {
                // Periodic need for remove visual on stun/fear/silence lost
                if (!(m_target->GetUInt32Value(UNIT_FIELD_FLAGS)&(UNIT_FLAG_STUNNED|UNIT_FLAG_FLEEING|UNIT_FLAG_SILENCED)))
                    m_target->RemoveAurasDueToSpell(52179);
                return;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Death and Decay
            if (spell->SpellFamilyFlags & UI64LIT(0x0000000000000020))
            {
                if (Unit *caster = GetCaster())
                    caster->CastCustomSpell(m_target, 52212, &m_modifier.m_amount, NULL, NULL, true, NULL, this);
                return;
            }
            // Raise Dead
//            if (spell->SpellFamilyFlags & UI64LIT(0x0000000000001000))
//                return;
            // Chains of Ice
            if (spell->SpellFamilyFlags & UI64LIT(0x0000400000000000))
            {
                // Get 0 effect aura
                Aura *slow = m_target->GetAura(GetId(), 0);
                if (slow)
                {
                    slow->ApplyModifier(false, true);
                    Modifier *mod = slow->GetModifier();
                    mod->m_amount+= m_modifier.m_amount;
                    if (mod->m_amount > 0) mod->m_amount = 0;
                    slow->ApplyModifier(true, true);
                }
                return;
            }
            // Summon Gargoyle
//            if (spell->SpellFamilyFlags & UI64LIT(0x0000008000000000))
//                return;
            // Death Rune Mastery
//            if (spell->SpellFamilyFlags & UI64LIT(0x0000000000004000))
//                return;
            // Bladed Armor
            if (spell->SpellIconID == 2653)
            {
                // Increases your attack power by $s1 for every $s2 armor value you have.
                // Calculate AP bonus (from 1 efect of this spell)
                int32 apBonus = m_modifier.m_amount * m_target->GetArmor() / m_target->CalculateSpellDamage(spell, 1, spell->EffectBasePoints[1], m_target);
                m_target->CastCustomSpell(m_target, 61217, &apBonus, &apBonus, NULL, true, NULL, this);
                return;
            }
            // Reaping
//            if (spell->SpellIconID == 22)
//                return;
            // Blood of the North
//            if (spell->SpellIconID == 30412)
//                return;
            break;
        }
        default:
            break;
    }
}

void Aura::HandlePreventFleeing(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit::AuraList const& fearAuras = m_target->GetAurasByType(SPELL_AURA_MOD_FEAR);
    if( !fearAuras.empty() )
    {
        if (apply)
            m_target->SetFeared(false, fearAuras.front()->GetCasterGUID());
        else
            m_target->SetFeared(true);
    }
}

void Aura::HandleManaShield(bool apply, bool Real)
{
    if(!Real)
        return;

    // prevent double apply bonuses
    if(apply && (m_target->GetTypeId()!=TYPEID_PLAYER || !((Player*)m_target)->GetSession()->PlayerLoading()))
    {
        if(Unit* caster = GetCaster())
        {
            float DoneActualBenefit = 0.0f;
            switch(m_spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_MAGE:
                    if(m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000008000))
                    {
                        // Mana Shield
                        // +50% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.5f;
                        break;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

void Aura::HandleArenaPreparation(bool apply, bool Real)
{
    if(!Real)
        return;

    if(apply)
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
    else
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
}

/**
 * Such auras are applied from a caster(=player) to a vehicle.
 * This has been verified using spell #49256
 */
void Aura::HandleAuraControlVehicle(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *player = GetCaster();
    Vehicle *vehicle = dynamic_cast<Vehicle*>(m_target);
    if(!player || player->GetTypeId() != TYPEID_PLAYER || !vehicle)
        return;

    if (apply)
    {
        if(Pet *pet = player->GetPet())
            pet->Remove(PET_SAVE_AS_CURRENT);
        ((Player*)player)->EnterVehicle(vehicle);
    }
    else
    {
        SpellEntry const *spell = GetSpellProto();

        // some SPELL_AURA_CONTROL_VEHICLE auras have a dummy effect on the player - remove them
        player->RemoveAurasDueToSpell(spell->Id);

        ((Player*)player)->ExitVehicle(vehicle);
    }
}

void Aura::HandleAuraConvertRune(bool apply, bool Real)
{
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *plr = (Player*)m_target;

    if(plr->getClass() != CLASS_DEATH_KNIGHT)
        return;

    // how to determine what rune need to be converted?
    for(uint32 i = 0; i < MAX_RUNES; ++i)
    {
        if(apply)
        {
            if(!plr->GetRuneCooldown(i))
            {
                plr->ConvertRune(i, RuneType(GetSpellProto()->EffectMiscValueB[m_effIndex]));
                break;
            }
        }
        else
        {
            if(plr->GetCurrentRune(i) == RuneType(GetSpellProto()->EffectMiscValueB[m_effIndex]))
            {
                plr->ConvertRune(i, plr->GetBaseRune(i));
                break;
            }
        }
    }
}

void Aura::HandlePhase(bool apply, bool Real)
{
    if(!Real)
        return;

    // always non stackable
    if(apply)
    {
        Unit::AuraList const& phases = m_target->GetAurasByType(SPELL_AURA_PHASE);
        if(!phases.empty())
            m_target->RemoveAurasDueToSpell(phases.front()->GetId(), this);
    }

    // no-phase is also phase state so same code for apply and remove

    // phase auras normally not expected at BG but anyway better check
    if(m_target->GetTypeId() == TYPEID_PLAYER)
    {
        // drop flag at invisible in bg
        if(((Player*)m_target)->InBattleGround())
            if(BattleGround *bg = ((Player*)m_target)->GetBattleGround())
                bg->EventPlayerDroppedFlag((Player*)m_target);

        // GM-mode have mask 0xFFFFFFFF
        if(!((Player*)m_target)->isGameMaster())
            m_target->SetPhaseMask(apply ? GetMiscValue() : PHASEMASK_NORMAL, false);

        ((Player*)m_target)->GetSession()->SendSetPhaseShift(apply ? GetMiscValue() : PHASEMASK_NORMAL);

        if(GetEffIndex()==0)
        {
            SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
            if(saBounds.first != saBounds.second)
            {
                uint32 zone, area;
                m_target->GetZoneAndAreaId(zone, area);

                for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
                {
                    // some auras remove at aura remove
                    if(!itr->second->IsFitToRequirements((Player*)m_target, zone, area))
                        m_target->RemoveAurasDueToSpell(itr->second->spellId);
                    // some auras applied at aura apply
                    else if(itr->second->autocast)
                    {
                        if( !m_target->HasAura(itr->second->spellId, 0) )
                            m_target->CastSpell(m_target, itr->second->spellId, true);
                    }
                }
            }
        }
    }
    else
        m_target->SetPhaseMask(apply ? GetMiscValue() : PHASEMASK_NORMAL, false);

    // need triggering visibility update base at phase update of not GM invisible (other GMs anyway see in any phases)
    if(m_target->GetVisibility() != VISIBILITY_OFF)
        m_target->SetVisibility(m_target->GetVisibility());
}

void Aura::UnregisterSingleCastAura()
{
    if (IsSingleTarget())
    {
        if(Unit* caster = GetCaster())
        {
            caster->GetSingleCastAuras().remove(this);
        }
        else
        {
            sLog.outError("Couldn't find the caster of the single target aura (SpellId %u), may crash later!", GetId());
            assert(false);
        }
        m_isSingleTargetAura = false;
    }
}

void Aura::HandleAuraSafeFall( bool Apply, bool Real )
{
    // implemented in WorldSession::HandleMovementOpcodes

    // only special case
    if(Apply && Real && GetId() == 32474 && m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->ActivateTaxiPathTo(506, GetId());
}

bool Aura::IsCritFromAbilityAura(Unit* caster, uint32& damage)
{
    Unit::AuraList const& auras = caster->GetAurasByType(SPELL_AURA_ABILITY_PERIODIC_CRIT);
    for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        if (!(*itr)->isAffectedOnSpell(m_spellProto))
            continue;
        if (!caster->isSpellCrit(m_target, m_spellProto, GetSpellSchoolMask(m_spellProto)))
            break;

        damage = caster->SpellCriticalDamageBonus(m_spellProto, damage, m_target);
        return true;
    }
    return false;
}

void Aura::HandleModTargetArmorPct(bool apply, bool Real)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateArmorPenetration();
}

void Aura::HandleAuraModAllCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (m_modifier.m_amount), apply);
    ((Player*)m_target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (m_modifier.m_amount), apply);
    ((Player*)m_target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (m_modifier.m_amount), apply);

    // included in Player::UpdateSpellCritChance calculation
    ((Player*)m_target)->UpdateAllSpellCritChances();
}

void Aura::HandleAllowOnlyAbility(bool apply, bool Real)
{
    if(!Real)
       return;

    if(apply)
    {
       m_target->setAttackTimer(BASE_ATTACK,m_duration);
       m_target->setAttackTimer(RANGED_ATTACK,m_duration);
       m_target->setAttackTimer(OFF_ATTACK,m_duration);
    }
    else
    {
       m_target->resetAttackTimer(BASE_ATTACK);
       m_target->resetAttackTimer(RANGED_ATTACK);
       m_target->resetAttackTimer(OFF_ATTACK);
    }

    m_target->UpdateDamagePhysical(BASE_ATTACK);
    m_target->UpdateDamagePhysical(RANGED_ATTACK);
    m_target->UpdateDamagePhysical(OFF_ATTACK);
}
