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
    &Aura::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN   implemented in Unit::MeleeDamageBonusTaken and Unit::SpellBaseDamageBonusTaken
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
    &Aura::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonusDone and Unit::SpellDamageBonusDone
    &Aura::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &Aura::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &Aura::HandlePeriodicHealthFunnel,                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
    &Aura::HandleUnused,                                    // 63 unused (3.0.8a-3.2.2a) old SPELL_AURA_PERIODIC_MANA_FUNNEL
    &Aura::HandlePeriodicManaLeech,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH
    &Aura::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &Aura::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &Aura::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &Aura::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &Aura::HandleSchoolAbsorb,                              // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalculateAbsorbAndResist
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
    &Aura::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT       implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &Aura::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &Aura::HandleModRegen,                                  // 84 SPELL_AURA_MOD_REGEN
    &Aura::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN
    &Aura::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &Aura::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonusTaken and Unit::SpellDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
    &Aura::HandlePeriodicDamagePCT,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &Aura::HandleUnused,                                    // 90 unused (3.0.8a-3.2.2a) old SPELL_AURA_MOD_RESIST_CHANCE
    &Aura::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &Aura::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &Aura::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &Aura::HandleNoImmediateEffect,                         // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::RegenerateAll
    &Aura::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &Aura::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
    &Aura::HandleManaShield,                                // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &Aura::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &Aura::HandleUnused,                                    //100 SPELL_AURA_AURAS_VISIBLE obsolete 3.x? all player can see all auras now, but still have 2 spells including GM-spell (1852,2855)
    &Aura::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &Aura::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &Aura::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &Aura::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &Aura::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &Aura::HandleAddModifier,                               //107 SPELL_AURA_ADD_FLAT_MODIFIER
    &Aura::HandleAddModifier,                               //108 SPELL_AURA_ADD_PCT_MODIFIER
    &Aura::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &Aura::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
    &Aura::HandleNoImmediateEffect,                         //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER implemented in Unit::SelectMagnetTarget
    &Aura::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS implemented in diff functions.
    &Aura::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusTaken
    &Aura::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT     imppemented in Player::RegenerateAll and Player::RegenerateHealth
    &Aura::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonusTaken
    &Aura::HandleUnused,                                    //119 unused (3.0.8a-3.2.2a) old SPELL_AURA_SHARE_PET_TRACKING
    &Aura::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &Aura::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &Aura::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &Aura::HandleModTargetResistance,                       //123 SPELL_AURA_MOD_TARGET_RESISTANCE
    &Aura::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &Aura::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &Aura::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &Aura::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &Aura::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &Aura::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &Aura::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &Aura::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &Aura::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonusDone
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
    &Aura::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT          implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleNoImmediateEffect,                         //154 SPELL_AURA_MOD_STEALTH_LEVEL          implemented in Unit::isVisibleForOrDetect
    &Aura::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING        implemented in Player::getMaxTimer
    &Aura::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN        implemented in Player::CalculateReputationGain
    &Aura::HandleUnused,                                    //157 SPELL_AURA_PET_DAMAGE_MULTI (single test like spell 20782, also single for 214 aura)
    &Aura::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &Aura::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT              implemented in Player::RewardHonor
    &Aura::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT implemented in Player::RegenerateAll and Player::RegenerateHealth
    &Aura::HandleAuraPowerBurn,                             //162 SPELL_AURA_POWER_BURN_MANA
    &Aura::HandleNoImmediateEffect,                         //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS      implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
    &Aura::HandleUnused,                                    //164 unused (3.0.8a-3.2.2a), only one test spell 10654
    &Aura::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &Aura::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &Aura::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonusDone, Unit::MeleeDamageBonusDone
    &Aura::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
    &Aura::HandleNULL,                                      //170 SPELL_AURA_DETECT_AMORE       different spells that ignore transformation effects
    &Aura::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &Aura::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &Aura::HandleUnused,                                    //173 unused (3.0.8a-3.2.2a) no spells, old SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &Aura::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonusDone
    &Aura::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonusDone
    &Aura::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &Aura::HandleNULL,                                      //177 SPELL_AURA_AOE_CHARM (22 spells)
    &Aura::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &Aura::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonusDone
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
    &Aura::HandleModCombatSpeedPct,                         //193 SPELL_AURA_HASTE_ALL (in fact combat (any type attack) speed pct)
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
    &Aura::HandleAuraModIncreaseFlightSpeed,                //206 SPELL_AURA_MOD_FLIGHT_SPEED
    &Aura::HandleAuraModIncreaseFlightSpeed,                //207 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED
    &Aura::HandleAuraModIncreaseFlightSpeed,                //208 SPELL_AURA_MOD_FLIGHT_SPEED_STACKING
    &Aura::HandleAuraModIncreaseFlightSpeed,                //209 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_STACKING
    &Aura::HandleAuraModIncreaseFlightSpeed,                //210 SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING
    &Aura::HandleAuraModIncreaseFlightSpeed,                //211 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_NOT_STACKING
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
    &Aura::HandleNoImmediateEffect,                         //229 SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE        implemented in Unit::SpellDamageBonusTaken
    &Aura::HandleAuraModIncreaseMaxHealth,                  //230 Commanding Shout
    &Aura::HandleNoImmediateEffect,                         //231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
    &Aura::HandleNoImmediateEffect,                         //232 SPELL_AURA_MECHANIC_DURATION_MOD           implement in Unit::CalculateSpellDuration
    &Aura::HandleNULL,                                      //233 set model id to the one of the creature with id m_modifier.m_miscvalue
    &Aura::HandleNoImmediateEffect,                         //234 SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK implement in Unit::CalculateSpellDuration
    &Aura::HandleAuraModDispelResist,                       //235 SPELL_AURA_MOD_DISPEL_RESIST               implement in Unit::MagicSpellHitResult
    &Aura::HandleAuraControlVehicle,                        //236 SPELL_AURA_CONTROL_VEHICLE
    &Aura::HandleModSpellDamagePercentFromAttackPower,      //237 SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER  implemented in Unit::SpellBaseDamageBonusDone
    &Aura::HandleModSpellHealingPercentFromAttackPower,     //238 SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER implemented in Unit::SpellBaseHealingBonusDone
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
    &Aura::HandleModCombatSpeedPct,                         //252 SPELL_AURA_SLOW_ALL
    &Aura::HandleNoImmediateEffect,                         //253 SPELL_AURA_MOD_BLOCK_CRIT_CHANCE             implemented in Unit::CalculateMeleeDamage
    &Aura::HandleNULL,                                      //254 SPELL_AURA_MOD_DISARM_SHIELD disarm Shield
    &Aura::HandleNoImmediateEffect,                         //255 SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT    implemented in Unit::SpellDamageBonusTaken
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
    &Aura::HandleNoImmediateEffect,                         //271 SPELL_AURA_MOD_DAMAGE_FROM_CASTER    implemented in Unit::SpellDamageBonusTaken
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
    &Aura::HandleNoImmediateEffect,                         //283 SPELL_AURA_MOD_HEALING_RECEIVED       implemented in Unit::SpellHealingBonusTaken
    &Aura::HandleNULL,                                      //284 51 spells
    &Aura::HandleAuraModAttackPowerOfArmor,                 //285 SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR  implemented in Player::UpdateAttackPowerAndDamage
    &Aura::HandleNoImmediateEffect,                         //286 SPELL_AURA_ABILITY_PERIODIC_CRIT      implemented in Aura::IsCritFromAbilityAura called from Aura::PeriodicTick
    &Aura::HandleNoImmediateEffect,                         //287 SPELL_AURA_DEFLECT_SPELLS             implemented in Unit::MagicSpellHitResult and Unit::MeleeSpellHitResult
    &Aura::HandleNULL,                                      //288 increase parry/deflect, prevent attack (single spell used 67801)
    &Aura::HandleUnused,                                    //289 unused (3.2.2a)
    &Aura::HandleAuraModAllCritChance,                      //290 SPELL_AURA_MOD_ALL_CRIT_CHANCE
    &Aura::HandleNoImmediateEffect,                         //291 SPELL_AURA_MOD_QUEST_XP_PCT           implemented in Player::GiveXP
    &Aura::HandleAuraOpenStable,                            //292 call stabled pet
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
    &Aura::HandleNULL,                                      //306 1 spell
    &Aura::HandleNULL,                                      //307 absorb healing?
    &Aura::HandleNULL,                                      //308 new aura for hunter traps
    &Aura::HandleNULL,                                      //309 absorb healing?
    &Aura::HandleNULL,                                      //310 pet avoidance passive?
    &Aura::HandleNULL,                                      //311 0 spells in 3.3
    &Aura::HandleNULL,                                      //312 0 spells in 3.3
    &Aura::HandleNULL,                                      //313 0 spells in 3.3
    &Aura::HandleNULL,                                      //314 1 test spell (reduce duration of silince/magic)
    &Aura::HandleNULL,                                      //315 underwater walking
    &Aura::HandleNULL                                       //316 makes haste affect HOT/DOT ticks
};

static AuraType const frozenAuraTypes[] = { SPELL_AURA_MOD_ROOT, SPELL_AURA_MOD_STUN, SPELL_AURA_NONE };

Aura::Aura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem) :
m_spellmod(NULL),
m_timeCla(1000), m_periodicTimer(0), m_periodicTick(0), m_removeMode(AURA_REMOVE_BY_DEFAULT),
m_effIndex(eff), m_spellAuraHolder(holder), m_isPersistent(false),
m_positive(false), m_isPeriodic(false), m_isAreaAura(false), m_in_use(0)
{
    MANGOS_ASSERT(target);

    MANGOS_ASSERT(spellproto && spellproto == sSpellStore.LookupEntry( spellproto->Id ) && "`info` must be pointer to sSpellStore element");

    m_currentBasePoints = currentBasePoints ? *currentBasePoints : spellproto->CalculateSimpleValue(eff);

    bool isPassive = IsPassiveSpell(GetSpellProto());
    bool isPermanent = false;
    m_positive = IsPositiveEffect(spellproto->Id, m_effIndex);
    uint64 caster_guid = !caster ? target->GetGUID() : caster->GetGUID();

    m_applyTime = time(NULL);

    int32 damage;
    if(!caster)
    {
        damage = m_currentBasePoints;
        m_maxduration = target->CalculateSpellDuration(spellproto, m_effIndex, target);
    }
    else
    {
        damage        = caster->CalculateSpellDamage(target, spellproto, m_effIndex, &m_currentBasePoints);
        m_maxduration = caster->CalculateSpellDuration(spellproto, m_effIndex, target);

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
                            if(pEnchant->spellid[t] == spellproto->Id)
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

    if(m_maxduration == -1 || isPassive && spellproto->DurationIndex == 0)
        isPermanent = true;

    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;

    if(!isPermanent && modOwner)
    {
        modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_DURATION, m_maxduration);
        // Get zero duration aura after - need set m_maxduration > 0 for apply/remove aura work
        if (m_maxduration<=0)
            m_maxduration = 1;
    }

    m_duration = m_maxduration;

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Aura: construct Spellid : %u, Aura : %u Duration : %d Target : %d Damage : %d", spellproto->Id, spellproto->EffectApplyAuraName[eff], m_maxduration, spellproto->EffectImplicitTargetA[eff],damage);

    SetModifier(AuraType(spellproto->EffectApplyAuraName[eff]), damage, spellproto->EffectAmplitude[eff], spellproto->EffectMiscValue[eff]);

    // Apply periodic time mod
    if(modOwner && m_modifier.periodictime)
        modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_ACTIVATION_TIME, m_modifier.periodictime);

    // Start periodic on next tick or at aura apply
    if (!(spellproto->AttributesEx5 & SPELL_ATTR_EX5_START_PERIODIC_AT_APPLY))
        m_periodicTimer += m_modifier.periodictime;
}

Aura::~Aura()
{
}

AreaAura::AreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isAreaAura = true;

    // caster==NULL in constructor args if target==caster in fact
    Unit* caster_ptr = caster ? caster : target;

    m_radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellproto->EffectRadiusIndex[m_effIndex]));
    if(Player* modOwner = caster_ptr->GetSpellModOwner())
        modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_RADIUS, m_radius);

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
            else if (target == caster_ptr && spellproto->Id == 53651)
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
            MANGOS_ASSERT(false);
            break;
    }
}

AreaAura::~AreaAura()
{
}

PersistentAreaAura::PersistentAreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isPersistent = true;
}

PersistentAreaAura::~PersistentAreaAura()
{
}

SingleEnemyTargetAura::SingleEnemyTargetAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
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
    return ObjectAccessor::GetUnit(*(m_spellAuraHolder->GetTarget()), m_casters_target_guid);
}

Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem)
{
    if (IsAreaAuraEffect(spellproto->Effect[eff]))
        return new AreaAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);

    uint32 triggeredSpellId = spellproto->EffectTriggerSpell[eff];

    if(SpellEntry const* triggeredSpellInfo = sSpellStore.LookupEntry(triggeredSpellId))
        for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
            if (triggeredSpellInfo->EffectImplicitTargetA[i] == TARGET_SINGLE_ENEMY)
                return new SingleEnemyTargetAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);

    return new Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);
}

SpellAuraHolder* CreateSpellAuraHolder(SpellEntry const* spellproto, Unit *target, WorldObject *caster, Item *castItem)
{
    return new SpellAuraHolder(spellproto, target, caster, castItem);
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
        if (GetEffIndex() == EFFECT_INDEX_0 && m_timeCla <= 0)
        {
            if(Unit* caster = GetCaster())
            {
                Powers powertype = Powers(GetSpellProto()->powerType);
                int32 manaPerSecond = GetSpellProto()->manaPerSecond + GetSpellProto()->manaPerSecondPerLevel * caster->getLevel();
                m_timeCla = 1*IN_MILLISECONDS;
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

    if(m_isPeriodic && (m_duration >= 0 || GetHolder()->IsPassive() || GetHolder()->IsPermanent()))
    {
        m_periodicTimer -= diff;
        if(m_periodicTimer <= 0) // tick also at m_periodicTimer==0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            // update before applying (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_modifier.periodictime;
            ++m_periodicTick;                               // for some infinity auras in some cases can overflow and reset
            PeriodicTick();
        }
    }
}

void AreaAura::Update(uint32 diff)
{
    // update for the caster of the aura
    if(GetCasterGUID() == GetTarget()->GetGUID())
    {
        Unit* caster = GetTarget();

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
                    MaNGOS::AnyFriendlyUnitInObjectRangeCheck u_check(caster, m_radius);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck> searcher(caster,targets, u_check);
                    Cell::VisitAllObjects(caster, searcher, m_radius);
                    break;
                }
                case AREA_AURA_ENEMY:
                {
                    MaNGOS::AnyAoETargetUnitInObjectRangeCheck u_check(caster, m_radius); // No GetCharmer in searcher
                    MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck> searcher(caster, targets, u_check);
                    Cell::VisitAllObjects(caster, searcher, m_radius);
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
                Unit::SpellAuraHolderBounds spair = (*tIter)->GetSpellAuraHolderBounds(GetId());
                for(Unit::SpellAuraHolderMap::const_iterator i = spair.first; i != spair.second; ++i)
                {
                    if (i->second->IsDeleted())
                        continue;

                    Aura *aur = i->second->GetAuraByEffectIndex(m_effIndex);

                    if (!aur)
                        continue;

                    switch(m_areaAuraType)
                    {
                        case AREA_AURA_ENEMY:
                            // non caster self-casted auras (non stacked)
                            if(aur->GetModifier()->m_auraname != SPELL_AURA_NONE)
                                apply = false;
                            break;
                        case AREA_AURA_RAID:
                            // non caster self-casted auras (stacked from diff. casters)
                            if(aur->GetModifier()->m_auraname != SPELL_AURA_NONE  || i->second->GetCasterGUID() == GetCasterGUID())
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

                if(SpellEntry const *actualSpellInfo = sSpellMgr.SelectAuraRankForLevel(GetSpellProto(), (*tIter)->getLevel()))
                {
                    int32 actualBasePoints = m_currentBasePoints;
                    // recalculate basepoints for lower rank (all AreaAura spell not use custom basepoints?)
                    if(actualSpellInfo != GetSpellProto())
                        actualBasePoints = actualSpellInfo->CalculateSimpleValue(m_effIndex);

                    SpellAuraHolder *holder = (*tIter)->GetSpellAuraHolder(actualSpellInfo->Id, GetCasterGUID());

                    bool addedToExisting = true;
                    if (!holder)
                    {
                        holder = CreateSpellAuraHolder(actualSpellInfo, (*tIter), caster);
                        addedToExisting = false;
                    }

                    AreaAura *aur = new AreaAura(actualSpellInfo, m_effIndex, &actualBasePoints, holder, (*tIter), caster, NULL);
                    aur->SetAuraDuration(GetAuraDuration());
                    holder->AddAura(aur, m_effIndex);

                    if (addedToExisting)
                    {
                        (*tIter)->AddAuraToModList(aur);
                        holder->SetInUse(true);
                        aur->ApplyModifier(true,true);
                        holder->SetInUse(false);
                    }
                    else
                        (*tIter)->AddSpellAuraHolder(holder);
                }
            }
        }
        Aura::Update(diff);
    }
    else                                                    // aura at non-caster
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();

        Aura::Update(diff);

        // remove aura if out-of-range from caster (after teleport for example)
        // or caster is isolated or caster no longer has the aura
        // or caster is (no longer) friendly
        bool needFriendly = (m_areaAuraType == AREA_AURA_ENEMY ? false : true);
        if( !caster || caster->hasUnitState(UNIT_STAT_ISOLATED) ||
            !caster->IsWithinDistInMap(target, m_radius)      ||
            !caster->HasAura(GetId(), GetEffIndex())            ||
            caster->IsFriendlyTo(target) != needFriendly
           )
        {
            target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(),GetCasterGUID());
        }
        else if( m_areaAuraType == AREA_AURA_PARTY)         // check if in same sub group
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGUID() != target->GetGUID() && caster->GetGUID() != target->GetCharmerOrOwnerGUID())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if( pGroup )
                {
                    Player* checkTarget = target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if(!checkTarget || !pGroup->SameSubGroup(check, checkTarget))
                        target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(),GetCasterGUID());
                }
                else
                    target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(),GetCasterGUID());
            }
        }
        else if( m_areaAuraType == AREA_AURA_RAID)          // TODO: fix me!
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGUID() != target->GetGUID() && caster->GetGUID() != target->GetCharmerOrOwnerGUID())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if( pGroup )
                {
                    Player* checkTarget = target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if(!checkTarget)
                        target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGUID());
                }
                else
                    target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGUID());
            }
        }
        else if( m_areaAuraType == AREA_AURA_PET || m_areaAuraType == AREA_AURA_OWNER )
        {
            if( target->GetGUID() != caster->GetCharmerOrOwnerGUID() )
                target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGUID());
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
            if (!GetTarget()->IsWithinDistInMap(dynObj, dynObj->GetRadius()))
                remove = true;
        }
        else
            remove = true;
    }
    else
        remove = true;

    Aura::Update(diff);

    if(remove)
        GetTarget()->RemoveAura(GetId(), GetEffIndex());
}

void Aura::ApplyModifier(bool apply, bool Real)
{
    AuraType aura = m_modifier.m_auraname;

    GetHolder()->SetInUse(true);
    SetInUse(true);
    if(aura < TOTAL_AURAS)
        (*this.*AuraHandler [aura])(apply, Real);
    SetInUse(false);
    GetHolder()->SetInUse(false);
}

bool Aura::isAffectedOnSpell(SpellEntry const *spell) const
{
    // Check family name
    if (spell->SpellFamilyName != GetSpellProto()->SpellFamilyName)
        return false;
    // Check EffectClassMask
    uint32 const *ptr = getAuraSpellClassMask();
    if (((uint64*)ptr)[0] & spell->SpellFamilyFlags)
        return true;
    if (ptr[2] & spell->SpellFamilyFlags2)
        return true;
    return false;
}

bool Aura::CanProcFrom(SpellEntry const *spell, uint32 EventProcEx, uint32 procEx, bool active) const
{
    // Check EffectClassMask
    uint32 const *ptr = getAuraSpellClassMask();

    // if no class mask defined - allow proc
    if (!((uint64*)ptr)[0] && !ptr[2])
    {
        if (IsPassiveSpell(GetSpellProto()) && !(EventProcEx & PROC_EX_EX_TRIGGER_ALWAYS))
        {
            // Check for extra req (if none) and hit/crit
            if (EventProcEx == PROC_EX_NONE)
            {
                // No extra req, so can trigger only for active (damage/healing present) and hit/crit
                if((procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) && active)
                    return true;
                else
                    return false;
            }
            else // Passive spells hits here only if resist/reflect/immune/evade
            {
                // Passive spells can`t trigger if need hit (exclude cases when procExtra include non-active flags)
                if ((EventProcEx & PROC_EX_NORMAL_HIT & procEx) && !active)
                    return false;
            }
        }
        return true;
    }
    else
    {
        // SpellFamilyName check is performed in SpellMgr::IsSpellProcEventCanTriggeredBy and it is done once for whole holder
        // note: SpellFamilyName is not checked if no spell_proc_event is defined

        if (((uint64*)ptr)[0] & spell->SpellFamilyFlags)
            return true;

        if (ptr[2] & spell->SpellFamilyFlags2)
            return true;
    }
    return false;
}

void Aura::ReapplyAffectedPassiveAuras( Unit* target, bool owner_mode )
{
    std::set<uint32> affectedSelf;
    std::set<uint32> affectedAuraCaster;

    for(Unit::SpellAuraHolderMap::const_iterator itr = target->GetSpellAuraHolderMap().begin(); itr != target->GetSpellAuraHolderMap().end(); ++itr)
    {
        // permanent passive or permanent area aura
        // passive spells can be affected only by own or owner spell mods)
        if (itr->second->IsPermanent() && (owner_mode && itr->second->IsPassive() /*|| itr->second->IsAreaAura()*/) &&
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
        target->CastSpell(GetTarget(), *set_itr, true);
    }

    if (!affectedAuraCaster.empty())
    {
        Unit* caster = GetCaster();
        for(std::set<uint32>::const_iterator set_itr = affectedAuraCaster.begin(); set_itr != affectedAuraCaster.end(); ++set_itr)
        {
            target->RemoveAurasDueToSpell(*set_itr);
            if (caster)
                caster->CastSpell(GetTarget(), *set_itr, true);
        }
    }
}

struct ReapplyAffectedPassiveAurasHelper
{
    explicit ReapplyAffectedPassiveAurasHelper(Aura* _aura) : aura(_aura) {}
    void operator()(Unit* unit) const { aura->ReapplyAffectedPassiveAuras(unit, true); }
    Aura* aura;
};

void Aura::ReapplyAffectedPassiveAuras()
{
    // not reapply spell mods with charges (use original value because processed and at remove)
    if (GetSpellProto()->procCharges)
        return;

    // not reapply some spell mods ops (mostly speedup case)
    switch (m_modifier.m_miscvalue)
    {
        case SPELLMOD_DURATION:
        case SPELLMOD_CHARGES:
        case SPELLMOD_NOT_LOSE_CASTING_TIME:
        case SPELLMOD_CASTING_TIME:
        case SPELLMOD_COOLDOWN:
        case SPELLMOD_COST:
        case SPELLMOD_ACTIVATION_TIME:
        case SPELLMOD_CASTING_TIME_OLD:
            return;
    }

    // reapply talents to own passive persistent auras
    ReapplyAffectedPassiveAuras(GetTarget(), true);

    // re-apply talents/passives/area auras applied to pet/totems (it affected by player spellmods)
    GetTarget()->CallForAllControlledUnits(ReapplyAffectedPassiveAurasHelper(this),true,false,false);

    // re-apply talents/passives/area auras applied to group members (it affected by player spellmods)
    if (Group* group = ((Player*)GetTarget())->GetGroup())
        for(GroupReference *itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            if (Player* member = itr->getSource())
                if (member != GetTarget() && member->IsInMap(GetTarget()))
                    ReapplyAffectedPassiveAuras(member, false);
}

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER || !Real)
        return;

    if(m_modifier.m_miscvalue >= MAX_SPELLMOD)
        return;

    if (apply)
    {
        // Add custom charges for some mod aura
        switch (GetSpellProto()->Id)
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
                GetHolder()->SetAuraCharges(1);
                break;
        }

        m_spellmod = new SpellModifier(
            SpellModOp(m_modifier.m_miscvalue),
            SpellModType(m_modifier.m_auraname),            // SpellModType value == spell aura types
            m_modifier.m_amount,
            this,
            // prevent expire spell mods with (charges > 0 && m_stackAmount > 1)
            // all this spell expected expire not at use but at spell proc event check
            GetSpellProto()->StackAmount > 1 ? 0 : GetHolder()->GetAuraCharges());
    }

    ((Player*)GetTarget())->AddSpellMod(m_spellmod, apply);

    ReapplyAffectedPassiveAuras();
}

void Aura::TriggerSpell()
{
    const uint64& casterGUID = GetCasterGUID();
    Unit* triggerTarget = GetTriggerTarget();

    if (!casterGUID || !triggerTarget)
        return;

    // generic casting code with custom spells and target/caster customs
    uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];

    SpellEntry const *triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
    SpellEntry const *auraSpellInfo = GetSpellProto();
    uint32 auraId = auraSpellInfo->Id;
    Unit* target = GetTarget();

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == NULL)
    {
        switch(auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch(auraId)
                {
                    case 812:                               // Periodic Mana Burn
                    {
                        trigger_spell_id = 25779;           // Mana Burn

                        // expected selection current fight target
                        triggerTarget = GetTarget()->getVictim();
                        if (!triggerTarget || triggerTarget->GetMaxPower(POWER_MANA) <= 0)
                            return;

                        triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
                        if (!triggeredSpellInfo)
                            return;

                        SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(triggeredSpellInfo->rangeIndex);
                        float max_range = GetSpellMaxRange(srange);
                        if (!triggerTarget->IsWithinDist(GetTarget(),max_range))
                            return;

                        break;
                    }
//                    // Polymorphic Ray
//                    case 6965: break;
                    case 9712:                              // Thaumaturgy Channel
                        trigger_spell_id = 21029;
                        break;
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
                    case 23170:                             // Brood Affliction: Bronze
                    {
                        target->CastSpell(target, 23171, true, NULL, this);
                        return;
                    }
//                    // Mark of Frost
//                    case 23184: break;
                    case 23493:                             // Restoration
                    {
                        int32 heal = triggerTarget->GetMaxHealth() / 10;
                        triggerTarget->DealHeal(triggerTarget, heal, auraSpellInfo);

                        if (int32 mana = triggerTarget->GetMaxPower(POWER_MANA))
                        {
                            mana /= 10;
                            triggerTarget->EnergizeBySpell(triggerTarget, 23493, mana, POWER_MANA);
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
                    case 24834:                             // Shadow Bolt Whirl
                    {
                        uint32 spellForTick[8] = { 24820, 24821, 24822, 24823, 24835, 24836, 24837, 24838 };
                        uint32 tick = GetAuraTicks();
                        if(tick < 8)
                        {
                            trigger_spell_id = spellForTick[tick];

                            // casted in left/right (but triggered spell have wide forward cone)
                            float forward = target->GetOrientation();
                            float angle = target->GetOrientation() + ( tick % 2 == 0 ? M_PI_F / 2 : - M_PI_F / 2);
                            target->SetOrientation(angle);
                            triggerTarget->CastSpell(triggerTarget, trigger_spell_id, true, NULL, this, casterGUID);
                            target->SetOrientation(forward);
                        }
                        return;
                    }
//                    // Stink Trap
//                    case 24918: break;
//                    // Mark of Nature
//                    case 25041: break;
//                    // Agro Drones
//                    case 25152: break;
                    case 25371:                             // Consume
                    {
                        int32 bpDamage = triggerTarget->GetMaxHealth()*10/100;
                        triggerTarget->CastCustomSpell(triggerTarget, 25373, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
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
                    case 27808:                             // Frost Blast
                    {
                        int32 bpDamage = triggerTarget->GetMaxHealth()*26/100;
                        triggerTarget->CastCustomSpell(triggerTarget, 29879, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
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
                    case 29528:                             // Inoculate Nestlewood Owlkin
                        // prevent error reports in case ignored player target
                        if (triggerTarget->GetTypeId() != TYPEID_UNIT)
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
                    case 29917:                             // Feed Captured Animal
                        trigger_spell_id = 29916;
                        break;
//                    // Flame Wreath
//                    case 29946: break;
//                    // Flame Wreath
//                    case 29947: break;
//                    // Mind Exhaustion Passive
//                    case 30025: break;
//                    // Nether Beam - Serenity
//                    case 30401: break;
                    case 30427:                             // Extract Gas
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                            return;
                        // move loot to player inventory and despawn target
                        if (caster->GetTypeId() ==TYPEID_PLAYER &&
                           triggerTarget->GetTypeId() == TYPEID_UNIT &&
                           ((Creature*)triggerTarget)->GetCreatureInfo()->type == CREATURE_TYPE_GAS_CLOUD)
                        {
                            Player* player = (Player*)caster;
                            Creature* creature = (Creature*)triggerTarget;
                            // missing lootid has been reported on startup - just return
                            if (!creature->GetCreatureInfo()->SkinLootId)
                                return;

                            player->AutoStoreLoot(creature->GetCreatureInfo()->SkinLootId,LootTemplates_Skinning,true);

                            creature->ForcedDespawn();
                        }
                        return;
                    }
                    case 30576:                             // Quake
                        trigger_spell_id = 30571;
                        break;
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
                    case 31347:                             // Doom
                    {
                        target->CastSpell(target,31350,true);
                        target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                        return;
                    }
                    case 31373:                             // Spellcloth
                    {
                        // Summon Elemental after create item
                        triggerTarget->SummonCreature(17870, 0, 0, 0, triggerTarget->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                        return;
                    }
//                    // Bloodmyst Tesla
//                    case 31611: break;
                    case 31944:                             // Doomfire
                    {
                        int32 damage = m_modifier.m_amount * ((GetAuraDuration() + m_modifier.periodictime) / GetAuraMaxDuration());
                        triggerTarget->CastCustomSpell(triggerTarget, 31969, &damage, NULL, NULL, true, NULL, this, casterGUID);
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
                    case 34229:                             // Flame Quills
                    {
                        // cast 24 spells 34269-34289, 34314-34316
                        for(uint32 spell_id = 34269; spell_id != 34290; ++spell_id)
                            triggerTarget->CastSpell(triggerTarget, spell_id, true, NULL, this, casterGUID);
                        for(uint32 spell_id = 34314; spell_id != 34317; ++spell_id)
                            triggerTarget->CastSpell(triggerTarget, spell_id, true, NULL, this, casterGUID);
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
                    case 37027:                             // Remote Toy
                        trigger_spell_id = 37029;
                        break;
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
                    case 38495:                             // Eye of Grillok
                    {
                        target->CastSpell(target, 38530, true);
                        return;
                    }
                    case 38554:                             // Absorb Eye of Grillok (Zezzak's Shard)
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                            return;

                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 38495, true, NULL, this);
                        else
                            return;

                        Creature* creatureTarget = (Creature*)target;

                        creatureTarget->ForcedDespawn();
                        return;
                    }
//                    // Magic Sucker Device timer
//                    case 38672: break;
//                    // Tomb Guarding Charging
//                    case 38751: break;
//                    // Murmur's Touch
//                    case 38794: break;
                    case 39105:                             // Activate Nether-wraith Beacon (31742 Nether-wraith Beacon item)
                    {
                        float fX, fY, fZ;
                        triggerTarget->GetClosePoint(fX, fY, fZ, triggerTarget->GetObjectBoundingRadius(), 20.0f);
                        triggerTarget->SummonCreature(22408, fX, fY, fZ, triggerTarget->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                        return;
                    }
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
                    case 39857:                             // Tear of Azzinoth Summon Channel - it's not really supposed to do anything,and this only prevents the console spam
                        trigger_spell_id = 39856;
                        break;
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
                    case 46736:                             // Personalized Weather
                        trigger_spell_id = 46737;
                        break;
//                    // Stay Submerged
//                    case 46981: break;
//                    // Dragonblight Ram
//                    case 47015: break;
//                    // Party G.R.E.N.A.D.E.
//                    case 51510: break;
//                    // Horseman Abilities
//                    case 52347: break;
//                    // GPS (Greater drake Positioning System)
//                    case 53389: break;
//                    // WotLK Prologue Frozen Shade Summon Aura
//                    case 53459: break;
//                    // WotLK Prologue Frozen Shade Speech
//                    case 53460: break;
//                    // WotLK Prologue Dual-plagued Brain Summon Aura
//                    case 54295: break;
//                    // WotLK Prologue Dual-plagued Brain Speech
//                    case 54299: break;
//                    // Rotate 360 (Fast)
//                    case 55861: break;
//                    // Shadow Sickle
//                    case 56702: break;
//                    // Portal Periodic
//                    case 58008: break;
//                    // Destroy Door Seal
//                    case 58040: break;
//                    // Draw Magic
//                    case 58185: break;
//                    // Food
//                    case 58886: break;
//                    // Shadow Sickle
//                    case 59103: break;
//                    // Time Bomb
//                    case 59376: break;
//                    // Whirlwind Visual
//                    case 59551: break;
//                    // Hearstrike
//                    case 59783: break;
//                    // Z Check
//                    case 61678: break;
//                    // isDead Check
//                    case 61976: break;
//                    // Start the Engine
//                    case 62432: break;
//                    // Enchanted Broom
//                    case 62571: break;
//                    // Mulgore Hatchling
//                    case 62586: break;
//                    // Durotar Scorpion
//                    case 62679: break;
//                    // Fighting Fish
//                    case 62833: break;
//                    // Shield Level 1
//                    case 63130: break;
//                    // Shield Level 2
//                    case 63131: break;
//                    // Shield Level 3
//                    case 63132: break;
//                    // Food
//                    case 64345: break;
//                    // Remove Player from Phase
//                    case 64445: break;
//                    // Food
//                    case 65418: break;
//                    // Food
//                    case 65419: break;
//                    // Food
//                    case 65420: break;
//                    // Food
//                    case 65421: break;
//                    // Food
//                    case 65422: break;
//                    // Rolling Throw
//                    case 67546: break;
//                    // Gunship Cannon Fire
//                    case 70017: break;
//                    // Ice Tomb
//                    case 70157: break;
//                    // Mana Barrier
//                    case 70842: break;
//                    // Summon Timer: Suppresser
//                    case 70912: break;
//                    // Aura of Darkness
//                    case 71110: break;
//                    // Aura of Darkness
//                    case 71111: break;
//                    // Ball of Flames Visual
//                    case 71706: break;
//                    // Summon Broken Frostmourne
//                    case 74081: break;
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                switch(auraId)
                {
                    case 66:                                // Invisibility
                        // Here need periodic trigger reducing threat spell (or do it manually)
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
//                break;
//            }
            case SPELLFAMILY_HUNTER:
            {
                switch (auraId)
                {
                    case 53302:                             // Sniper training
                    case 53303:
                    case 53304:
                        if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                            return;

                        // Reset reapply counter at move
                        if (((Player*)triggerTarget)->isMoving())
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
                        if (triggerTarget->HasAura(trigger_spell_id))
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
                    case 768:                               // Cat Form
                        // trigger_spell_id not set and unknown effect triggered in this case, ignoring for while
                        return;
                    case 22842:                             // Frenzied Regeneration
                    case 22895:
                    case 22896:
                    case 26999:
                    {
                        int32 LifePerRage = GetModifier()->m_amount;

                        int32 lRage = target->GetPower(POWER_RAGE);
                        if (lRage > 100)                    // rage stored as rage*10
                            lRage = 100;
                        target->ModifyPower(POWER_RAGE, -lRage);
                        int32 FRTriggerBasePoints = int32(lRage*LifePerRage/10);
                        target->CastCustomSpell(target, 22845, &FRTriggerBasePoints, NULL, NULL, true, NULL, this);
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
                    case 28820:                             // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
                    {
                        // Need remove self if Lightning Shield not active
                        Unit::SpellAuraHolderMap const& auras = triggerTarget->GetSpellAuraHolderMap();
                        for(Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            SpellEntry const* spell = itr->second->GetSpellProto();
                            if (spell->SpellFamilyName == SPELLFAMILY_SHAMAN &&
                                (spell->SpellFamilyFlags & UI64LIT(0x0000000000000400)))
                                return;
                        }
                        triggerTarget->RemoveAurasDueToSpell(28820);
                        return;
                    }
                    case 38443:                             // Totemic Mastery (Skyshatter Regalia (Shaman Tier 6) - bonus)
                    {
                        if (triggerTarget->IsAllTotemSlotsUsed())
                            triggerTarget->CastSpell(triggerTarget, 38437, true, NULL, this);
                        else
                            triggerTarget->RemoveAurasDueToSpell(38437);
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
            case 9347:                                      // Mortal Strike
            {
                // expected selection current fight target
                triggerTarget = GetTarget()->getVictim();
                if (!triggerTarget)
                    return;

                // avoid triggering for far target
                SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(triggeredSpellInfo->rangeIndex);
                float max_range = GetSpellMaxRange(srange);
                if (!triggerTarget->IsWithinDist(GetTarget(),max_range))
                    return;

                break;
            }
            case 1010:                                      // Curse of Idiocy
            {
                // TODO: spell casted by result in correct way mostly
                // BUT:
                // 1) target show casting at each triggered cast: target don't must show casting animation for any triggered spell
                //      but must show affect apply like item casting
                // 2) maybe aura must be replace by new with accumulative stat mods instead stacking

                // prevent cast by triggered auras
                if (casterGUID == triggerTarget->GetGUID())
                    return;

                // stop triggering after each affected stats lost > 90
                int32 intelectLoss = 0;
                int32 spiritLoss = 0;

                Unit::AuraList const& mModStat = triggerTarget->GetAurasByType(SPELL_AURA_MOD_STAT);
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

                if (intelectLoss <= -90 && spiritLoss <= -90)
                    return;

                break;
            }
            case 16191:                                     // Mana Tide
            {
                triggerTarget->CastCustomSpell(triggerTarget, trigger_spell_id, &m_modifier.m_amount, NULL, NULL, true, NULL, this);
                return;
            }
            case 33525:                                     // Ground Slam
                triggerTarget->CastSpell(triggerTarget, trigger_spell_id, true, NULL, this, casterGUID);
                return;
            case 38736:                                     // Rod of Purification - for quest 10839 (Veil Skith: Darkstone of Terokk)
            {
                if (Unit* caster = GetCaster())
                    caster->CastSpell(triggerTarget, trigger_spell_id, true, NULL, this);
                return;
            }
            case 53563:                                     // Beacon of Light
                // original caster must be target (beacon)
                target->CastSpell(target, trigger_spell_id, true, NULL, this, target->GetGUID());
                return;
            case 56654:                                     // Rapid Recuperation (triggered energize have baspioints == 0)
            case 58882:
            {
                int32 mana = target->GetMaxPower(POWER_MANA) * m_modifier.m_amount / 100;
                triggerTarget->CastCustomSpell(triggerTarget, trigger_spell_id, &mana, NULL, NULL, true, NULL, this);
                return;
            }
        }
    }

    // All ok cast by default case
    if (triggeredSpellInfo)
        triggerTarget->CastSpell(triggerTarget, triggeredSpellInfo, true, NULL, this, casterGUID);
    else
    {
        if (Unit* caster = GetCaster())
        {
            if (triggerTarget->GetTypeId() != TYPEID_UNIT || !Script->EffectDummyCreature(caster, GetId(), GetEffIndex(), (Creature*)triggerTarget))
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
    int32  basepoints0 = GetModifier()->m_amount;

    target->CastCustomSpell(target, trigger_spell_id, &basepoints0, NULL, NULL, true, NULL, this, casterGUID);
}

/*********************************************************/
/***                  AURA EFFECTS                     ***/
/*********************************************************/

void Aura::HandleAuraDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    // AT APPLY
    if (apply)
    {
        switch(GetSpellProto()->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch(GetId())
                {
                    case 1515:                              // Tame beast
                        // FIX_ME: this is 2.0.12 threat effect replaced in 2.1.x by dummy aura, must be checked for correctness
                        if (target->CanHaveThreatList())
                            if (Unit* caster = GetCaster())
                                target->AddThreat(caster, 10.0f, false, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
                        return;
                    case 7057:                              // Haunting Spirits
                        // expected to tick with 30 sec period (tick part see in Aura::PeriodicTick)
                        m_isPeriodic = true;
                        m_modifier.periodictime = 30*IN_MILLISECONDS;
                        m_periodicTimer = m_modifier.periodictime;
                        return;
                    case 10255:                             // Stoned
                    {
                        if (Unit* caster = GetCaster())
                        {
                            if (caster->GetTypeId() != TYPEID_UNIT)
                                return;

                            caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            caster->addUnitState(UNIT_STAT_ROOT);
                        }
                        return;
                    }
                    case 13139:                             // net-o-matic
                        // root to self part of (root_target->charge->root_self sequence
                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 13138, true, NULL, this);
                        return;
                    case 31606:                             // Stormcrow Amulet
                    {
                        CreatureInfo const * cInfo = ObjectMgr::GetCreatureTemplate(17970);

                        // we must assume db or script set display id to native at ending flight (if not, target is stuck with this model)
                        if (cInfo)
                            target->SetDisplayId(Creature::ChooseDisplayId(cInfo));

                        return;
                    }
                    case 32045:                             // Soul Charge
                    case 32051:
                    case 32052:
                    {
                        // max duration is 2 minutes, but expected to be random duration
                        // real time randomness is unclear, using max 30 seconds here
                        // see further down for expire of this aura
                        SetAuraDuration(rand()%30*IN_MILLISECONDS);
                        return;
                    }
                    // Gender spells
                    case 38224:                             // Illidari Agent Illusion
                    case 37096:                             // Blood Elf Illusion
                    case 46354:                             // Blood Elf Illusion
                    {
                        uint8 gender = target->getGender();
                        uint32 spellId;
                        switch (GetId())
                        {
                            case 38224: spellId = (gender == GENDER_MALE ? 38225 : 38227); break;
                            case 37096: spellId = (gender == GENDER_MALE ? 37092 : 37094); break;
                            case 46354: spellId = (gender == GENDER_MALE ? 46355 : 46356); break;
                            default: return;
                        }
                        target->CastSpell(target, spellId, true, NULL, this);
                        return;
                    }
                    case 39850:                             // Rocket Blast
                        if (roll_chance_i(20))              // backfire stun
                            target->CastSpell(target, 51581, true, NULL, this);
                        return;
                    case 43873:                             // Headless Horseman Laugh
                        target->PlayDistanceSound(11965);
                        return;
                    case 46699:                             // Requires No Ammo
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            // not use ammo and not allow use
                            ((Player*)target)->RemoveAmmo();
                        return;
                    case 47190:                             // Toalu'u's Spiritual Incense
                        target->CastSpell(target, 47189, true, NULL, this);
                        // allow script to process further (text)
                        break;
                    case 48025:                             // Headless Horseman's Mount
                        Spell::SelectMountByAreaAndSkill(target, 51621, 48024, 51617, 48023, 0);
                        return;
                    case 62061:                             // Festive Holiday Mount
                        if (target->HasAuraType(SPELL_AURA_MOUNTED))
                            // Reindeer Transformation
                            target->CastSpell(target, 25860, true, NULL, this);
                        return;
                    case 63624:                             // Learn a Second Talent Specialization
                        // Teach Learn Talent Specialization Switches, required for client triggered casts, allow after 30 sec delay
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            ((Player*)target)->learnSpell(63680, false);
                        return;
                    case 63651:                             // Revert to One Talent Specialization
                        // Teach Learn Talent Specialization Switches, remove
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            ((Player*)target)->removeSpell(63680);
                        return;
                    case 71342:                             // Big Love Rocket
                        Spell::SelectMountByAreaAndSkill(target, 71344, 71345, 71346, 71347, 0);
                        return;
                    case 72286:                             // Invincible
                        Spell::SelectMountByAreaAndSkill(target, 72281, 72282, 72283, 72284, 0);
                        return;
                    case 74856:                             // Blazing Hippogryph
                        Spell::SelectMountByAreaAndSkill(target, 0, 0, 74854, 74855, 0);
                        return;
                    case 75614:                             // Celestial Steed
                        Spell::SelectMountByAreaAndSkill(target, 75619, 75620, 75617, 75618, 76153);
                        return;
                }
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                // Overpower
                if (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000004))
                {
                    // Must be casting target
                    if (!target->IsNonMeleeSpellCasted(false))
                        return;

                    Unit* caster = GetCaster();
                    if (!caster)
                        return;

                    Unit::AuraList const& modifierAuras = caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for(Unit::AuraList::const_iterator itr = modifierAuras.begin(); itr != modifierAuras.end(); ++itr)
                    {
                        // Unrelenting Assault
                        if ((*itr)->GetSpellProto()->SpellFamilyName==SPELLFAMILY_WARRIOR && (*itr)->GetSpellProto()->SpellIconID == 2775)
                        {
                            switch ((*itr)->GetSpellProto()->Id)
                            {
                                case 46859:                 // Unrelenting Assault, rank 1
                                    target->CastSpell(target,64849,true,NULL,(*itr));
                                    break;
                                case 46860:                 // Unrelenting Assault, rank 2
                                    target->CastSpell(target,64850,true,NULL,(*itr));
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
            }
            case SPELLFAMILY_SHAMAN:
            {
                // Tidal Force
                if (GetId() == 55198)
                {
                    // apply max stack bufs
                    SpellEntry const* buffEntry = sSpellStore.LookupEntry(55166);
                    if (!buffEntry)
                        return;

                    for(uint32 k = 0; k < buffEntry->StackAmount; ++k)
                        target->CastSpell(target, buffEntry, true, NULL, this);
                }
                // Earth Shield
                else if ((GetSpellProto()->SpellFamilyFlags & UI64LIT(0x40000000000)))
                {
                    // prevent double apply bonuses
                    if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
                    {
                        if (Unit* caster = GetCaster())
                        {
                            m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                            m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                        }
                    }
                    return;
                }
                break;
            }
        }
    }
    // AT REMOVE
    else
    {
        if (IsQuestTameSpell(GetId()) && target->isAlive())
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

            if (finalSpelId)
                caster->CastSpell(target, finalSpelId, true, NULL, this);

            return;
        }

        switch(GetId())
        {
            case 10255:                                     // Stoned
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetTypeId() != TYPEID_UNIT)
                        return;

                    // see dummy effect of spell 10254 for removal of flags etc
                    caster->CastSpell(caster, 10254, true);
                }
                return;
            }
            case 12479:                                     // Hex of Jammal'an
                target->CastSpell(target, 12480, true, NULL, this);
                return;
            case 28169:                                     // Mutating Injection
            {
                // Mutagen Explosion
                target->CastSpell(target, 28206, true, NULL, this);
                // Poison Cloud
                target->CastSpell(target, 28240, true, NULL, this);
                return;
            }
            case 32045:                                     // Soul Charge
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                    target->CastSpell(target, 32054, true, NULL, this);

                return;
            }
            case 32051:                                     // Soul Charge
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                    target->CastSpell(target, 32057, true, NULL, this);

                return;
            }
            case 32052:                                     // Soul Charge
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                    target->CastSpell(target, 32053, true, NULL, this);

                return;
            }
            case 32286:                                     // Focus Target Visual
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                    target->CastSpell(target, 32301, true, NULL, this);

                return;
            }
            case 36730:                                     // Flame Strike
            {
                target->CastSpell(target, 36731, true, NULL, this);
                return;
            }
            case 44191:                                     // Flame Strike
            {
                if (target->GetMap()->IsDungeon())
                {
                    uint32 spellId = target->GetMap()->IsRegularDifficulty() ? 44190 : 46163;

                    target->CastSpell(target, spellId, true, NULL, this);
                }
                return;
            }
            case 45934:                                     // Dark Fiend
            {
                // Kill target if dispelled
                if (m_removeMode==AURA_REMOVE_BY_DISPEL)
                    target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                return;
            }
            case 46308:                                     // Burning Winds
            {
                // casted only at creatures at spawn
                target->CastSpell(target, 47287, true, NULL, this);
                return;
            }
            case 51870:                                     // Collect Hair Sample
            {
                if (Unit* pCaster = GetCaster())
                {
                    if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                        pCaster->CastSpell(target, 51872, true, NULL, this);
                }

                return;
            }
            case 58600:                                     // Restricted Flight Area
            {
                AreaTableEntry const* area = GetAreaEntryByAreaID(target->GetAreaId());

                // Dalaran restricted flight zone (recheck before apply unmount)
                if (area && target->GetTypeId() == TYPEID_PLAYER && (area->flags & AREA_FLAG_CANNOT_FLY) &&
                    ((Player*)target)->IsFreeFlying() && !((Player*)target)->isGameMaster())
                {
                    target->CastSpell(target, 58601, true); // Remove Flight Auras (also triggered Parachute (45472))
                }
                return;
            }
        }

        // Living Bomb
        if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_MAGE && (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x2000000000000)))
        {
            // Zero duration is equal to AURA_REMOVE_BY_DEFAULT. We can't use it directly, as it is set even
            // when removing aura from one target due to casting Living Bomb at other.
            if (m_duration == 0 || m_removeMode == AURA_REMOVE_BY_DISPEL)
                target->CastSpell(target,m_modifier.m_amount,true,NULL,this);

            return;
        }
    }

    // AT APPLY & REMOVE

    switch(GetSpellProto()->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch(GetId())
            {
                case 11196:                                 // Recently Bandaged
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, GetMiscValue(), apply);
                    return;
                case 24658:                                 // Unstable Power
                {
                    uint32 spellId = 24659;
                    if (apply)
                    {
                        SpellEntry const *spell = sSpellStore.LookupEntry(spellId);
                        Unit* caster = GetCaster();
                        if (!spell || !caster)
                            return;

                        for (uint32 i = 0; i < spell->StackAmount; ++i)
                            caster->CastSpell(target, spellId, true, NULL, NULL, GetCasterGUID());

                        return;
                    }
                    target->RemoveAurasDueToSpell(spellId);
                    return;
                }
                case 24661:                                 // Restless Strength
                {
                    uint32 spellId = 24662;
                    if (apply)
                    {
                        SpellEntry const* spell = sSpellStore.LookupEntry(spellId);
                        Unit* caster = GetCaster();
                        if (!spell || !caster)
                            return;

                        for (uint32 i=0; i < spell->StackAmount; ++i)
                            caster->CastSpell(target, spell->Id, true, NULL, NULL, GetCasterGUID());

                        return;
                    }
                    target->RemoveAurasDueToSpell(spellId);
                    return;
                }
                case 29266:                                 // Permanent Feign Death
                case 31261:                                 // Permanent Feign Death (Root)
                case 37493:                                 // Feign Death
                case 51329:                                 // Feign Death
                case 52593:                                 // Bloated Abomination Feign Death
                case 55795:                                 // Falling Dragon Feign Death
                case 57626:                                 // Feign Death
                case 57685:                                 // Permanent Feign Death
                case 58768:                                 // Permanent Feign Death (Freeze Jumpend)
                case 58806:                                 // Permanent Feign Death (Drowned Anim)
                case 58951:                                 // Permanent Feign Death
                case 64461:                                 // Permanent Feign Death (No Anim) (Root)
                case 65985:                                 // Permanent Feign Death (Root Silence Pacify)
                case 70592:                                 // Permanent Feign Death
                case 70628:                                 // Permanent Feign Death
                case 70630:                                 // Frozen Aftermath - Feign Death
                case 71598:                                 // Feign Death
                {
                    // Unclear what the difference really is between them.
                    // Some has effect1 that makes the difference, however not all.
                    // Some appear to be used depending on creature location, in water, at solid ground, in air/suspended, etc
                    // For now, just handle all the same way
                    if (target->GetTypeId() == TYPEID_UNIT)
                        target->SetFeignDeath(apply);

                    return;
                }
                case 35356:                                 // Spawn Feign Death
                case 35357:                                 // Spawn Feign Death
                case 42557:                                 // Feign Death
                {
                    if (target->GetTypeId() == TYPEID_UNIT)
                    {
                        // Flags not set like it's done in SetFeignDeath() and apparently always applied at spawn of creature
                        // All three does however have SPELL_EFFECT_SPAWN(46) as effect1
                        // It is possible this effect will remove some flags, and then the three here can be handled "normally"
                        if (apply)
                        {
                            target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                            target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

                            target->addUnitState(UNIT_STAT_DIED);
                        }
                        else
                        {
                            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                            target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

                            target->clearUnitState(UNIT_STAT_DIED);
                        }
                    }
                    return;
                }
                case 40133:                                 //Summon Fire Elemental
                {
                    Unit* caster = GetCaster();
                    if (!caster)
                        return;

                    Unit *owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (apply)
                            owner->CastSpell(owner, 8985, true);
                        else
                            ((Player*)owner)->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
                    }
                    return;
                }
                case 40132:                                 //Summon Earth Elemental
                {
                    Unit* caster = GetCaster();
                    if (!caster)
                        return;

                    Unit *owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (apply)
                            owner->CastSpell(owner, 19704, true);
                        else
                            ((Player*)owner)->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
                    }
                    return;
                }
                case 40214:                                 //Dragonmaw Illusion
                {
                    if (apply)
                    {
                        target->CastSpell(target, 40216, true);
                        target->CastSpell(target, 42016, true);
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(40216);
                        target->RemoveAurasDueToSpell(42016);
                    }
                    return;
                }
                case 58204:                                 // LK Intro VO (1)
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 1
                        if (apply)
                            target->PlayDirectSound(14970, (Player *)target);
                        // continue in 58205
                        else
                            target->CastSpell(target, 58205, true);
                    }
                    return;
                case 58205:                                 // LK Intro VO (2)
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 2
                        if (apply)
                            target->PlayDirectSound(14971, (Player *)target);
                        // Play part 3
                        else
                            target->PlayDirectSound(14972, (Player *)target);
                    }
                    return;
                case 40131:
                case 27978:
                    if (apply)
                        target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    else
                        target->m_AuraFlags |= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
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
                        target->CastCustomSpell(caster, 48210, &bp0, NULL, NULL, true, NULL, this);
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Pain and Suffering
            if (GetSpellProto()->SpellIconID == 2874 && target->GetTypeId()==TYPEID_PLAYER)
            {
                if (apply)
                {
                    // Reduce backfire damage (dot damage) from Shadow Word: Death
                    // aura have wrong effectclassmask, so use hardcoded value
                    m_spellmod = new SpellModifier(SPELLMOD_DOT,SPELLMOD_PCT,m_modifier.m_amount,GetId(),UI64LIT(0x0000200000000000));
                }
                ((Player*)target)->AddSpellMod(m_spellmod, apply);
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
                    if (target->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (apply)
                        // dummy not have proper effectclassmask
                        m_spellmod  = new SpellModifier(SPELLMOD_DOT,SPELLMOD_FLAT,m_modifier.m_amount/7,GetId(),UI64LIT(0x001000000000));

                    ((Player*)target)->AddSpellMod(m_spellmod, apply);
                    return;
                }
                case 52610:                                 // Savage Roar
                {
                    if (apply)
                    {
                        if (target->m_form != FORM_CAT)
                            return;

                        target->CastSpell(target, 62071, true);
                    }
                    else
                        target->RemoveAurasDueToSpell(62071);
                    return;
                }
                case 61336:                                 // Survival Instincts
                {
                    if(apply)
                    {
                        if (!target->IsInFeralForm())
                            return;

                        int32 bp0 = int32(target->GetMaxHealth() * m_modifier.m_amount / 100);
                        target->CastCustomSpell(target, 50322, &bp0, NULL, NULL, true);
                    }
                    else
                        target->RemoveAurasDueToSpell(50322);
                    return;
                }
            }

            // Lifebloom
            if (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x1000000000))
            {
                if (apply)
                {
                    if (Unit* caster = GetCaster())
                    {
                        // prevent double apply bonuses
                        if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
                        {
                            m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                            m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                        }
                    }
                }
                else
                {
                    // Final heal on duration end
                    if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
                        return;

                    // final heal
                    if (target->IsInWorld() && GetStackAmount() > 0)
                    {
                        int32 amount = m_modifier.m_amount;
                        target->CastCustomSpell(target, 33778, &amount, NULL, NULL, true, NULL, this, GetCasterGUID());

                        if (Unit* caster = GetCaster())
                        {
                            int32 returnmana = (GetSpellProto()->ManaCostPercentage * caster->GetCreateMana() / 100) * GetStackAmount() / 2;
                            caster->CastCustomSpell(caster, 64372, &returnmana, NULL, NULL, true, NULL, this, GetCasterGUID());
                        }
                    }
                }
                return;
            }

            // Predatory Strikes
            if (target->GetTypeId()==TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
            {
                ((Player*)target)->UpdateAttackPowerAndDamage();
                return;
            }

            // Improved Moonkin Form
            if (GetSpellProto()->SpellIconID == 2855)
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

                if (apply)
                {
                    if (target->m_form != FORM_MOONKIN)
                        return;

                    target->CastSpell(target, spell_id, true);
                }
                else
                    target->RemoveAurasDueToSpell(spell_id);
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
                        target->CastSpell(target, 67480, true, NULL, this);
                    else
                        target->RemoveAurasDueToSpell(67480);
                    return;
                }
            }
            break;
        case SPELLFAMILY_SHAMAN:
            break;
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr.GetPetAura(GetId(), m_effIndex))
    {
        if (apply)
            target->AddPetAura(petSpell);
        else
            target->RemovePetAura(petSpell);
        return;
    }

    if (GetEffIndex() == EFFECT_INDEX_0 && target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if (saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            target->GetZoneAndAreaId(zone, area);

            for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                // some auras remove at aura remove
                if (!itr->second->IsFitToRequirements((Player*)target, zone, area))
                    target->RemoveAurasDueToSpell(itr->second->spellId);
                // some auras applied at aura apply
                else if (itr->second->autocast)
                {
                    if (!target->HasAura(itr->second->spellId, EFFECT_INDEX_0))
                        target->CastSpell(target, itr->second->spellId, true);
                }
            }
        }
    }

    // script has to "handle with care", only use where data are not ok to use in the above code.
    if (target->GetTypeId() == TYPEID_UNIT)
        Script->EffectAuraDummy(this, apply);
}

void Aura::HandleAuraMounted(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(apply)
    {
        CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
        if(!ci)
        {
            sLog.outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need it modelid)", m_modifier.m_miscvalue);
            return;
        }

        uint32 display_id = Creature::ChooseDisplayId(ci);
        CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
        if (minfo)
            display_id = minfo->modelid;

        target->Mount(display_id, GetId());
    }
    else
    {
        target->Unmount();
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
    data << GetTarget()->GetPackGUID();
    data << uint32(0);
    GetTarget()->SendMessageToSet(&data, true);
}

void Aura::HandleAuraFeatherFall(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;
    Unit *target = GetTarget();
    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_FEATHER_FALL, 8+4);
    else
        data.Initialize(SMSG_MOVE_NORMAL_FALL, 8+4);
    data << target->GetPackGUID();
    data << uint32(0);
    target->SendMessageToSet(&data, true);

    // start fall from current height
    if(!apply && target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->SetFallInformation(0, target->GetPositionZ());
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
    data << GetTarget()->GetPackGUID();
    data << uint32(0);
    GetTarget()->SendMessageToSet(&data, true);
}

void Aura::HandleWaterBreathing(bool /*apply*/, bool /*Real*/)
{
    // update timers in client
    if(GetTarget()->GetTypeId()==TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateMirrorTimers();
}

void Aura::HandleAuraModShapeshift(bool apply, bool Real)
{
    if(!Real)
        return;

    uint32 modelid = 0;
    Powers PowerType = POWER_MANA;
    ShapeshiftForm form = ShapeshiftForm(m_modifier.m_miscvalue);

    Unit *target = GetTarget();

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
        if (target->GetTypeId() != TYPEID_PLAYER)
            modelid = ssEntry->modelID_A;
        else
        {
            // players are a bit different since the dbc has seldomly an horde modelid
            if (Player::TeamForRace(target->getRace()) == HORDE)
            {
                if (ssEntry->modelID_H)
                    modelid = ssEntry->modelID_H;           // 3.2.3 only the moonkin form has this information
                else                                        // get model for race
                    modelid = sObjectMgr.GetModelForRace(ssEntry->modelID_A, target->getRaceMask());
            }

            // nothing found in above, so use default
            if (!modelid)
                modelid = ssEntry->modelID_A;
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
            target->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
            Unit::AuraList const& slowingAuras = target->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
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
                target->RemoveAurasDueToSpellByCancel(aurSpellInfo->Id);
                iter = slowingAuras.begin();
            }

            // and polymorphic affects
            if(target->IsPolymorphed())
                target->RemoveAurasDueToSpell(target->getTransForm());

            break;
        }
        default:
           break;
    }

    if(apply)
    {
        // remove other shapeshift before applying a new one
        if(target->m_ShapeShiftFormSpellId)
            target->RemoveAurasDueToSpell(target->m_ShapeShiftFormSpellId, GetHolder());

        target->SetByteValue(UNIT_FIELD_BYTES_2, 3, form);

        if(modelid > 0)
            target->SetDisplayId(modelid);

        if(PowerType != POWER_MANA)
        {
            // reset power to default values only at power change
            if(target->getPowerType() != PowerType)
                target->setPowerType(PowerType);

            switch(form)
            {
                case FORM_CAT:
                case FORM_BEAR:
                case FORM_DIREBEAR:
                {
                    // get furor proc chance
                    int32 furorChance = 0;
                    Unit::AuraList const& mDummy = target->GetAurasByType(SPELL_AURA_DUMMY);
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
                        if ((int32)target->GetPower(POWER_ENERGY) > furorChance)
                        {
                            target->SetPower(POWER_ENERGY, 0);
                            target->CastCustomSpell(target, 17099, &furorChance, NULL, NULL, this);
                        }
                    }
                    else if(furorChance)                    // only if talent known
                    {
                        target->SetPower(POWER_RAGE, 0);
                        if(irand(1,100) <= furorChance)
                            target->CastSpell(target, 17057, true, NULL, this);
                    }
                    break;
                }
                case FORM_BATTLESTANCE:
                case FORM_DEFENSIVESTANCE:
                case FORM_BERSERKERSTANCE:
                {
                    uint32 Rage_val = 0;
                    // Stance mastery + Tactical mastery (both passive, and last have aura only in defense stance, but need apply at any stance switch)
                    if(target->GetTypeId() == TYPEID_PLAYER)
                    {
                        PlayerSpellMap const& sp_list = ((Player *)target)->GetSpellMap();
                        for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
                        {
                            if(itr->second.state == PLAYERSPELL_REMOVED) continue;
                            SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                            if (spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_WARRIOR && spellInfo->SpellIconID == 139)
                                Rage_val += target->CalculateSpellDamage(target, spellInfo, EFFECT_INDEX_0) * 10;
                        }
                    }

                    if (target->GetPower(POWER_RAGE) > Rage_val)
                        target->SetPower(POWER_RAGE, Rage_val);
                    break;
                }
                default:
                    break;
            }
        }

        target->m_ShapeShiftFormSpellId = GetId();
        target->m_form = form;

        // a form can give the player a new castbar with some spells.. this is a clientside process..
        // serverside just needs to register the new spells so that player isn't kicked as cheater
        if (target->GetTypeId() == TYPEID_PLAYER)
            for (uint32 i = 0; i < 8; ++i)
                if (ssEntry->spellId[i])
                    ((Player*)target)->addSpell(ssEntry->spellId[i], true, false, false, false);

    }
    else
    {
        if(modelid > 0)
            target->SetDisplayId(target->GetNativeDisplayId());
        target->SetByteValue(UNIT_FIELD_BYTES_2, 3, FORM_NONE);
        if(target->getClass() == CLASS_DRUID)
            target->setPowerType(POWER_MANA);
        target->m_ShapeShiftFormSpellId = 0;
        target->m_form = FORM_NONE;

        switch(form)
        {
            // Nordrassil Harness - bonus
            case FORM_BEAR:
            case FORM_DIREBEAR:
            case FORM_CAT:
                if(Aura* dummy = target->GetDummyAura(37315) )
                    target->CastSpell(target, 37316, true, NULL, dummy);
                break;
            // Nordrassil Regalia - bonus
            case FORM_MOONKIN:
                if(Aura* dummy = target->GetDummyAura(37324) )
                    target->CastSpell(target, 37325, true, NULL, dummy);
                break;
            default:
                break;
        }

        // look at the comment in apply-part
        if (target->GetTypeId() == TYPEID_PLAYER)
            for (uint32 i = 0; i < 8; ++i)
                if (ssEntry->spellId[i])
                    ((Player*)target)->removeSpell(ssEntry->spellId[i], false, false, false);

    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(apply);

    if(target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->InitDataForForm();
}

void Aura::HandleAuraTransform(bool apply, bool Real)
{
    Unit *target = GetTarget();
    if (apply)
    {
        // special case (spell specific functionality)
        if (m_modifier.m_miscvalue == 0)
        {
            // player applied only
            if (target->GetTypeId() != TYPEID_PLAYER)
                return;

            switch (GetId())
            {
                // Orb of Deception
                case 16739:
                {
                    uint32 orb_model = target->GetNativeDisplayId();
                    switch(orb_model)
                    {
                        // Troll Female
                        case 1479: target->SetDisplayId(10134); break;
                        // Troll Male
                        case 1478: target->SetDisplayId(10135); break;
                        // Tauren Male
                        case 59:   target->SetDisplayId(10136); break;
                        // Human Male
                        case 49:   target->SetDisplayId(10137); break;
                        // Human Female
                        case 50:   target->SetDisplayId(10138); break;
                        // Orc Male
                        case 51:   target->SetDisplayId(10139); break;
                        // Orc Female
                        case 52:   target->SetDisplayId(10140); break;
                        // Dwarf Male
                        case 53:   target->SetDisplayId(10141); break;
                        // Dwarf Female
                        case 54:   target->SetDisplayId(10142); break;
                        // NightElf Male
                        case 55:   target->SetDisplayId(10143); break;
                        // NightElf Female
                        case 56:   target->SetDisplayId(10144); break;
                        // Undead Female
                        case 58:   target->SetDisplayId(10145); break;
                        // Undead Male
                        case 57:   target->SetDisplayId(10146); break;
                        // Tauren Female
                        case 60:   target->SetDisplayId(10147); break;
                        // Gnome Male
                        case 1563: target->SetDisplayId(10148); break;
                        // Gnome Female
                        case 1564: target->SetDisplayId(10149); break;
                        // BloodElf Female
                        case 15475: target->SetDisplayId(17830); break;
                        // BloodElf Male
                        case 15476: target->SetDisplayId(17829); break;
                        // Dranei Female
                        case 16126: target->SetDisplayId(17828); break;
                        // Dranei Male
                        case 16125: target->SetDisplayId(17827); break;
                        default: break;
                    }
                    break;
                }
                // Murloc costume
                case 42365: target->SetDisplayId(21723); break;
                // Honor the Dead
                case 65386:
                case 65495:
                {
                    switch(target->getGender())
                    {
                        case GENDER_MALE:
                            target->SetDisplayId(29203);  // Chapman
                            break;
                        case GENDER_FEMALE:
                        case GENDER_NONE:
                            target->SetDisplayId(29204);  // Catrina
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
                model_id = Creature::ChooseDisplayId(ci);   // Will use the default model here

            // Polymorph (sheep/penguin case)
            if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_MAGE && GetSpellProto()->SpellIconID == 82)
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(52648))             // Glyph of the Penguin
                        model_id = 26452;

            target->SetDisplayId(model_id);

            // creature case, need to update equipment
            if (ci && target->GetTypeId() == TYPEID_UNIT)
                ((Creature*)target)->LoadEquipment(ci->equipmentId, true);

            // Dragonmaw Illusion (set mount model also)
            if(GetId()==42016 && target->GetMountID() && !target->GetAurasByType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED).empty())
                target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID,16314);
        }

        // update active transform spell only not set or not overwriting negative by positive case
        if (!target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(target->getTransForm()))
            target->setTransForm(GetId());

        // polymorph case
        if (Real && target->GetTypeId() == TYPEID_PLAYER && target->IsPolymorphed())
        {
            // for players, start regeneration after 1s (in polymorph fast regeneration case)
            // only if caster is Player (after patch 2.4.2)
            if (IS_PLAYER_GUID(GetCasterGUID()) )
                ((Player*)target)->setRegenTimer(1*IN_MILLISECONDS);

            //dismount polymorphed target (after patch 2.4.2)
            if (target->IsMounted())
                target->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
        }
    }
    else
    {
        // ApplyModifier(true) will reapply it if need
        target->setTransForm(0);
        target->SetDisplayId(target->GetNativeDisplayId());

        // apply default equipment for creature case
        if (target->GetTypeId() == TYPEID_UNIT)
            ((Creature*)target)->LoadEquipment(((Creature*)target)->GetCreatureInfo()->equipmentId, true);

        // re-apply some from still active with preference negative cases
        Unit::AuraList const& otherTransforms = target->GetAurasByType(SPELL_AURA_TRANSFORM);
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
        if (GetId() == 42016 && target->GetMountID() == 16314)
        {
            if (!target->GetAurasByType(SPELL_AURA_MOUNTED).empty())
            {
                uint32 cr_id = target->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetModifier()->m_miscvalue;
                if (CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(cr_id))
                {
                    uint32 display_id = Creature::ChooseDisplayId(ci);
                    CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
                    if (minfo)
                        display_id = minfo->modelid;

                    target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, display_id);
                }
            }
        }
    }
}

void Aura::HandleForceReaction(bool apply, bool Real)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if(!Real)
        return;

    Player* player = (Player*)GetTarget();

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
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 prot=GetSpellProto()->EffectMiscValue[m_effIndex];
    int32 points = GetModifier()->m_amount;

    ((Player*)GetTarget())->ModifySkillBonus(prot, (apply ? points: -points), m_modifier.m_auraname == SPELL_AURA_MOD_SKILL_TALENT);
    if(prot == SKILL_DEFENSE)
        ((Player*)GetTarget())->UpdateDefenseBonusesMod();
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

        // Soul Shard (target req.)
        if (spellInfo->EffectItemType[m_effIndex] == 6265)
        {
            // Only from non-grey units
            if (!((Player*)caster)->isHonorOrXPTarget(victim) ||
                victim->GetTypeId() == TYPEID_UNIT && !((Player*)caster)->isAllowedToLoot((Creature*)victim))
                return;
        }

        //Adding items
        uint32 noSpaceForCount = 0;
        uint32 count = m_modifier.m_amount;

        ItemPosCountVec dest;
        uint8 msg = ((Player*)caster)->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, spellInfo->EffectItemType[m_effIndex], count, &noSpaceForCount);
        if( msg != EQUIP_ERR_OK )
        {
            count-=noSpaceForCount;
            ((Player*)caster)->SendEquipError( msg, NULL, NULL, spellInfo->EffectItemType[m_effIndex] );
            if (count==0)
                return;
        }

        Item* newitem = ((Player*)caster)->StoreNewItem(dest, spellInfo->EffectItemType[m_effIndex], true);
        ((Player*)caster)->SendNewItem(newitem, count, true, true);

        // Soul Shard (glyph bonus)
        if (spellInfo->EffectItemType[m_effIndex] == 6265)
        {
            // Glyph of Soul Shard
            if (caster->HasAura(58070) && roll_chance_i(40))
                caster->CastSpell(caster, 58068, true, NULL, this);
        }
    }
}

void Aura::HandleBindSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Camera& camera = ((Player*)caster)->GetCamera();
    if (apply)
        camera.SetView(GetTarget());
    else
        camera.ResetView();
}

void Aura::HandleFarSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Camera& camera = ((Player*)caster)->GetCamera();
    if (apply)
        camera.SetView(GetTarget());
    else
        camera.ResetView();
}

void Aura::HandleAuraTrackCreatures(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId()!=TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue-1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue-1));
}

void Aura::HandleAuraTrackResources(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId()!=TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue-1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue-1));
}

void Aura::HandleAuraTrackStealthed(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    GetTarget()->ApplyModFlag(PLAYER_FIELD_BYTES, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void Aura::HandleAuraModScale(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, float(m_modifier.m_amount), apply);
    GetTarget()->UpdateModelData();
}

void Aura::HandleModPossess(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    // not possess yourself
    if(GetCasterGUID() == target->GetGUID())
        return;

    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)caster;
    Camera& camera = p_caster->GetCamera();

    if( apply )
    {
        target->addUnitState(UNIT_STAT_CONTROLLED);

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
        target->SetCharmerGUID(p_caster->GetGUID());
        target->setFaction(p_caster->getFaction());

        // target should became visible at SetView call(if not visible before):
        // otherwise client\p_caster will ignore packets from the target(SetClientControl for example)
        camera.SetView(target);

        p_caster->SetCharm(target);
        p_caster->SetClientControl(target, 1);
        p_caster->SetMover(target);

        target->CombatStop(true);
        target->DeleteThreatList();
        target->getHostileRefManager().deleteReferences();

        if(CharmInfo *charmInfo = target->InitCharmInfo(target))
        {
            charmInfo->InitPossessCreateSpells();
            charmInfo->SetReactState(REACT_PASSIVE);
            charmInfo->SetCommandState(COMMAND_STAY);
        }

        p_caster->PossessSpellInitialize();

        if(target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)target)->AIM_Initialize();
        }
        else if(target->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)target)->SetClientControl(target, 0);
        }

    }
    else
    {
        p_caster->SetCharm(NULL);

        p_caster->SetClientControl(target, 0);
        p_caster->SetMover(NULL);

        // there is a possibility that target became invisible for client\p_caster at ResetView call:
        // it must be called after movement control unapplying, not before! the reason is same as at aura applying
        camera.ResetView();

        p_caster->RemovePetActionBar();

        // on delete only do caster related effects
        if(m_removeMode == AURA_REMOVE_BY_DELETE)
            return;

        target->clearUnitState(UNIT_STAT_CONTROLLED);

        target->CombatStop(true);
        target->DeleteThreatList();
        target->getHostileRefManager().deleteReferences();

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        target->SetCharmerGUID(0);

        if(target->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)target)->setFactionForRace(target->getRace());
            ((Player*)target)->SetClientControl(target, 1);
        }
        else if(target->GetTypeId() == TYPEID_UNIT)
        {
            CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();
            target->setFaction(cinfo->faction_A);
        }

        if(target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)target)->AIM_Initialize();

            if (((Creature*)target)->AI())
                ((Creature*)target)->AI()->AttackedBy(caster);
        }
    }
}

void Aura::HandleModPossessPet(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* target = GetTarget();
    if (target->GetTypeId() != TYPEID_UNIT || !((Creature*)target)->isPet())
        return;

    Pet* pet = (Pet*)target;

    Player* p_caster = (Player*)caster;
    Camera& camera = p_caster->GetCamera();

    if (apply)
    {
        pet->addUnitState(UNIT_STAT_CONTROLLED);

        // target should became visible at SetView call(if not visible before):
        // otherwise client\p_caster will ignore packets from the target(SetClientControl for example)
        camera.SetView(pet);

        p_caster->SetCharm(pet);
        p_caster->SetClientControl(pet, 1);
        ((Player*)caster)->SetMover(pet);

        pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        pet->StopMoving();
        pet->GetMotionMaster()->Clear(false);
        pet->GetMotionMaster()->MoveIdle();
    }
    else
    {
        p_caster->SetCharm(NULL);
        p_caster->SetClientControl(pet, 0);
        p_caster->SetMover(NULL);

        // there is a possibility that target became invisible for client\p_caster at ResetView call:
        // it must be called after movement control unapplying, not before! the reason is same as at aura applying
        camera.ResetView();

        // on delete only do caster related effects
        if(m_removeMode == AURA_REMOVE_BY_DELETE)
            return;

        pet->clearUnitState(UNIT_STAT_CONTROLLED);

        pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        pet->AttackStop();

        // out of range pet dismissed
        if (!pet->IsWithinDistInMap(p_caster, pet->GetMap()->GetVisibilityDistance()))
        {
            pet->Remove(PET_SAVE_NOT_IN_SLOT, true);
        }
        else
        {
            pet->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            pet->AddSplineFlag(SPLINEFLAG_WALKMODE);
        }
    }
}

void Aura::HandleAuraModPetTalentsPoints(bool /*Apply*/, bool Real)
{
    if(!Real)
        return;

    // Recalculate pet talent points
    if (Pet *pet=GetTarget()->GetPet())
        pet->InitTalentForLevel();
}

void Aura::HandleModCharm(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    // not charm yourself
    if(GetCasterGUID() == target->GetGUID())
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    if( apply )
    {
        if (target->GetCharmerGUID())
        {
            target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM);
            target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS);
        }

        target->SetCharmerGUID(GetCasterGUID());
        target->setFaction(caster->getFaction());
        target->CastStop(target == caster ? GetId() : 0);
        caster->SetCharm(target);

        target->CombatStop(true);
        target->DeleteThreatList();
        target->getHostileRefManager().deleteReferences();

        if(target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)target)->AIM_Initialize();
            CharmInfo *charmInfo = target->InitCharmInfo(target);
            charmInfo->InitCharmCreateSpells();
            charmInfo->SetReactState( REACT_DEFENSIVE );

            if(caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK)
            {
                CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();
                if(cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                {
                    // creature with pet number expected have class set
                    if(target->GetByteValue(UNIT_FIELD_BYTES_0, 1)==0)
                    {
                        if(cinfo->unit_class==0)
                            sLog.outErrorDb("Creature (Entry: %u) have unit_class = 0 but used in charmed spell, that will be result client crash.",cinfo->Entry);
                        else
                            sLog.outError("Creature (Entry: %u) have unit_class = %u but at charming have class 0!!! that will be result client crash.",cinfo->Entry,cinfo->unit_class);

                        target->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
                    }

                    //just to enable stat window
                    charmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
                    //if charmed two demons the same session, the 2nd gets the 1st one's name
                    target->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
                }
            }
        }

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->CharmSpellInitialize();
    }
    else
    {
        target->SetCharmerGUID(0);

        if(target->GetTypeId() == TYPEID_PLAYER)
            ((Player*)target)->setFactionForRace(target->getRace());
        else
        {
            CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();

            // restore faction
            if(((Creature*)target)->isPet())
            {
                if(Unit* owner = target->GetOwner())
                    target->setFaction(owner->getFaction());
                else if(cinfo)
                    target->setFaction(cinfo->faction_A);
            }
            else if(cinfo)                              // normal creature
                target->setFaction(cinfo->faction_A);

            // restore UNIT_FIELD_BYTES_0
            if(cinfo && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK && cinfo->type == CREATURE_TYPE_DEMON)
            {
                // DB must have proper class set in field at loading, not req. restore, including workaround case at apply
                // m_target->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->unit_class);

                if(target->GetCharmInfo())
                    target->GetCharmInfo()->SetPetNumber(0, true);
                else
                    sLog.outError("Aura::HandleModCharm: target (GUID: %u TypeId: %u) has a charm aura but no charm info!", target->GetGUIDLow(), target->GetTypeId());
            }
        }

        caster->SetCharm(NULL);

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->RemovePetActionBar();

        target->CombatStop(true);
        target->DeleteThreatList();
        target->getHostileRefManager().deleteReferences();

        if(target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)target)->AIM_Initialize();
            if (((Creature*)target)->AI())
                ((Creature*)target)->AI()->AttackedBy(caster);
        }
    }
}

void Aura::HandleModConfuse(bool apply, bool Real)
{
    if(!Real)
        return;

    GetTarget()->SetConfused(apply, GetCasterGUID(), GetId());
}

void Aura::HandleModFear(bool apply, bool Real)
{
    if (!Real)
        return;

    GetTarget()->SetFeared(apply, GetCasterGUID(), GetId());
}

void Aura::HandleFeignDeath(bool apply, bool Real)
{
    if(!Real)
        return;

    GetTarget()->SetFeignDeath(apply, GetCasterGUID(), GetId());
}

void Aura::HandleAuraModDisarm(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(!apply && target->HasAuraType(SPELL_AURA_MOD_DISARM))
        return;

    // not sure for it's correctness
    if(apply)
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED);
    else
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED);

    // only at real add/remove aura
    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // main-hand attack speed already set to special value for feral form already and don't must change and reset at remove.
    if (target->IsInFeralForm())
        return;

    if (apply)
        target->SetAttackTime(BASE_ATTACK,BASE_ATTACK_TIME);
    else
        ((Player *)target)->SetRegularAttackTime();

    target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleAuraModStun(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(UNIT_STAT_STUNNED);
        target->SetTargetGUID(0);

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        target->CastStop(target->GetGUID() == GetCasterGUID() ? GetId() : 0);

        // Creature specific
        if(target->GetTypeId() != TYPEID_PLAYER)
            target->StopMoving();
        else
        {
            ((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
            target->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client
        }

        WorldPacket data(SMSG_FORCE_MOVE_ROOT, 8);
        data << target->GetPackGUID();
        data << uint32(0);
        target->SendMessageToSet(&data, true);

        // Summon the Naj'entus Spine GameObject on target if spell is Impaling Spine
        if(GetId() == 39837)
        {
            GameObject* pObj = new GameObject;
            if(pObj->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_GAMEOBJECT), 185584, target->GetMap(), target->GetPhaseMask(),
                target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
            {
                pObj->SetRespawnTime(GetAuraDuration()/IN_MILLISECONDS);
                pObj->SetSpellId(GetId());
                target->AddGameObject(pObj);
                target->GetMap()->Add(pObj);
            }
            else
                delete pObj;
        }
    }
    else
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for(AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
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
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if(target->HasAuraType(SPELL_AURA_MOD_STUN))
            return;

        target->clearUnitState(UNIT_STAT_STUNNED);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if(!target->hasUnitState(UNIT_STAT_ROOT))         // prevent allow move if have also root effect
        {
            if(target->getVictim() && target->isAlive())
                target->SetTargetGUID(target->getVictim()->GetGUID());

            WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8+4);
            data << target->GetPackGUID();
            data << uint32(0);
            target->SendMessageToSet(&data, true);
        }

        // Wyvern Sting
        if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000100000000000))
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

            caster->CastSpell(target,spellInfo,true,NULL,this);
            return;
        }
    }
}

void Aura::HandleModStealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (apply)
    {
        // drop flag at stealth in bg
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // only at real aura add
        if (Real)
        {
            target->SetStandFlags(UNIT_STAND_FLAGS_CREEP);

            if (target->GetTypeId()==TYPEID_PLAYER)
                target->SetFlag(PLAYER_FIELD_BYTES2, 0x2000);

            // apply only if not in GM invisibility (and overwrite invisibility state)
            if (target->GetVisibility()!=VISIBILITY_OFF)
            {
                target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                target->SetVisibility(VISIBILITY_GROUP_STEALTH);
            }

            // apply full stealth period bonuses only at first stealth aura in stack
            if(target->GetAurasByType(SPELL_AURA_MOD_STEALTH).size()<=1)
            {
                Unit::AuraList const& mDummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = mDummyAuras.begin();i != mDummyAuras.end(); ++i)
                {
                    // Master of Subtlety
                    if ((*i)->GetSpellProto()->SpellIconID == 2114)
                    {
                        target->RemoveAurasDueToSpell(31666);
                        int32 bp = (*i)->GetModifier()->m_amount;
                        target->CastCustomSpell(target,31665,&bp,NULL,NULL,true);
                    }
                    // Overkill
                    else if ((*i)->GetId() == 58426 && GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000400000))
                    {
                        target->CastSpell(target, 58427, true);
                    }
                }
            }
        }
    }
    else
    {
        // only at real aura remove of _last_ SPELL_AURA_MOD_STEALTH
        if (Real && !target->HasAuraType(SPELL_AURA_MOD_STEALTH))
        {
            // if no GM invisibility
            if (target->GetVisibility()!=VISIBILITY_OFF)
            {
                target->RemoveStandFlags(UNIT_STAND_FLAGS_CREEP);

                if (target->GetTypeId()==TYPEID_PLAYER)
                    target->RemoveFlag(PLAYER_FIELD_BYTES2, 0x2000);

                // restore invisibility if any
                if (target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
                {
                    target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                    target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
                }
                else
                    target->SetVisibility(VISIBILITY_ON);
            }

            // apply delayed talent bonus remover at last stealth aura remove
            Unit::AuraList const& mDummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
            for(Unit::AuraList::const_iterator i = mDummyAuras.begin();i != mDummyAuras.end(); ++i)
            {
                // Master of Subtlety
                if ((*i)->GetSpellProto()->SpellIconID == 2114)
                    target->CastSpell(target, 31666, true);
                // Overkill
                else if ((*i)->GetId() == 58426 && GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000400000))
                {
                    if (Aura* aura = target->GetAura(58427, EFFECT_INDEX_0))
                    {
                        aura->SetAuraMaxDuration(20*IN_MILLISECONDS);
                        aura->GetHolder()->RefreshHolder();
                    }
                }
            }
        }
    }
}

void Aura::HandleInvisibility(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if(apply)
    {
        target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);

        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        if(Real && target->GetTypeId()==TYPEID_PLAYER)
        {
            // apply glow vision
            target->SetFlag(PLAYER_FIELD_BYTES2,PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

        }

        // apply only if not in GM invisibility and not stealth
        if(target->GetVisibility() == VISIBILITY_ON)
        {
            // Aura not added yet but visibility code expect temporary add aura
            target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
            target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_invisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_invisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);

        // only at real aura remove and if not have different invisibility auras.
        if(Real && target->m_invisibilityMask == 0)
        {
            // remove glow vision
            if(target->GetTypeId() == TYPEID_PLAYER)
                target->RemoveFlag(PLAYER_FIELD_BYTES2,PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

            // apply only if not in GM invisibility & not stealthed while invisible
            if(target->GetVisibility() != VISIBILITY_OFF)
            {
                // if have stealth aura then already have stealth visibility
                if(!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                    target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibilityDetect(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if(apply)
    {
        target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_detectInvisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_detectInvisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);
    }
    if(Real && target->GetTypeId()==TYPEID_PLAYER)
        ((Player*)target)->GetCamera().UpdateVisibilityForOwner();
}

void Aura::HandleAuraModRoot(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(UNIT_STAT_ROOT);
        target->SetTargetGUID(0);

        //Save last orientation
        if( target->getVictim() )
            target->SetOrientation(target->GetAngle(target->getVictim()));

        if(target->GetTypeId() == TYPEID_PLAYER)
        {
            WorldPacket data(SMSG_FORCE_MOVE_ROOT, 10);
            data << target->GetPackGUID();
            data << (uint32)2;
            target->SendMessageToSet(&data, true);

            //Clear unit movement flags
            ((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
        }
        else
            target->StopMoving();
    }
    else
    {
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for(AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
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
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if(target->HasAuraType(SPELL_AURA_MOD_ROOT))
            return;

        target->clearUnitState(UNIT_STAT_ROOT);

        if(!target->hasUnitState(UNIT_STAT_STUNNED))      // prevent allow move if have also stun effect
        {
            if(target->getVictim() && target->isAlive())
                target->SetTargetGUID(target->getVictim()->GetGUID());

            if(target->GetTypeId() == TYPEID_PLAYER)
            {
                WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 10);
                data << target->GetPackGUID();
                data << (uint32)2;
                target->SendMessageToSet(&data, true);
            }
        }
    }
}

void Aura::HandleAuraModSilence(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(apply)
    {
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                if(spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    target->InterruptSpell(CurrentSpellTypes(i), false);
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if(target->HasAuraType(SPELL_AURA_MOD_SILENCE))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void Aura::HandleModThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive())
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
            level_diff = target->getLevel() - 60;
            multiplier = 2;
            break;
        // The Eye of Diminution
        case 28862:
            level_diff = target->getLevel() - 60;
            multiplier = 1;
            break;
    }

    if (level_diff > 0)
        m_modifier.m_amount += multiplier * level_diff;

    if (target->GetTypeId() == TYPEID_PLAYER)
        for(int8 x=0;x < MAX_SPELL_SCHOOL;x++)
            if (m_modifier.m_miscvalue & int32(1<<x))
                ApplyPercentModFloatVar(target->m_threatModifier[x], float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModTotalThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    float threatMod = apply ? float(m_modifier.m_amount) : float(-m_modifier.m_amount);

    target->getHostileRefManager().threatAssist(caster, threatMod, GetSpellProto());
}

void Aura::HandleModTaunt(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive() || !target->CanHaveThreatList())
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    if (apply)
        target->TauntApply(caster);
    else
    {
        // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
        target->TauntFadeOut(caster);
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

    GetTarget()->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseMountedSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);

    // Festive Holiday Mount
    if (apply && GetSpellProto()->SpellIconID != 1794 && target->HasAura(62061))
        // Reindeer Transformation
        target->CastSpell(target, 25860, true, NULL, this);
}

void Aura::HandleAuraModIncreaseFlightSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    // Enable Fly mode for flying mounts
    if (m_modifier.m_auraname == SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED)
    {
        WorldPacket data;
        if(apply)
            data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
        else
            data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
        data << target->GetPackGUID();
        data << uint32(0);                                      // unknown
        target->SendMessageToSet(&data, true);

        //Players on flying mounts must be immune to polymorph
        if (target->GetTypeId()==TYPEID_PLAYER)
            target->ApplySpellImmune(GetId(),IMMUNITY_MECHANIC,MECHANIC_POLYMORPH,apply);

        // Dragonmaw Illusion (overwrite mount model, mounted aura already applied)
        if (apply && target->HasAura(42016, EFFECT_INDEX_0) && target->GetMountID())
            target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID,16314);

        // Festive Holiday Mount
        if (apply && GetSpellProto()->SpellIconID != 1794 && target->HasAura(62061))
            // Reindeer Transformation
            target->CastSpell(target, 25860, true, NULL, this);
    }

    // Swift Flight Form check for higher speed flying mounts
    if (apply && target->GetTypeId() == TYPEID_PLAYER && GetSpellProto()->Id == 40121)
    {
        for (PlayerSpellMap::const_iterator iter = ((Player*)target)->GetSpellMap().begin(); iter != ((Player*)target)->GetSpellMap().end(); ++iter)
        {
            if (iter->second.state != PLAYERSPELL_REMOVED)
            {
                bool changedSpeed = false;
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(iter->first);
                for(int i = 0; i < MAX_EFFECT_INDEX; ++i)
                {
                    if(spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED)
                    {
                        int32 mountSpeed = spellInfo->CalculateSimpleValue(SpellEffectIndex(i));
                        if (mountSpeed > m_modifier.m_amount)
                        {
                            m_modifier.m_amount = mountSpeed;
                            changedSpeed = true;
                            break;
                        }
                    }
                }
                if (changedSpeed)
                    break;
            }
        }
    }

    target->UpdateSpeed(MOVE_FLIGHT, true);
}

void Aura::HandleAuraModIncreaseSwimSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModDecreaseSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        // Gronn Lord's Grasp, becomes stoned
        if (GetId() == 33572)
        {
            if (GetStackAmount() >= 5 && !target->HasAura(33652))
                target->CastSpell(target, 33652, true);
        }
    }

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
    target->UpdateSpeed(MOVE_FLIGHT, true);
}

void Aura::HandleAuraModUseNormalSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
    target->UpdateSpeed(MOVE_FLIGHT, true);
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

    Unit *target = GetTarget();

    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        uint32 mechanic = 1 << (misc-1);

        //immune movement impairment and loss of control
        if(GetId()==42292 || GetId()==59752 || GetId()==65547)
            mechanic=IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

        target->RemoveAurasAtMechanicImmunity(mechanic,GetId());
    }

    target->ApplySpellImmune(GetId(),IMMUNITY_MECHANIC,misc,apply);

    // Bestial Wrath
    if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellIconID == 1680)
    {
        // The Beast Within cast on owner if talent present
        if (Unit* owner = target->GetOwner())
        {
            // Search talent The Beast Within
            Unit::AuraList const& dummyAuras = owner->GetAurasByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
            for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
            {
                if ((*i)->GetSpellProto()->SpellIconID == 2229)
                {
                    if (apply)
                        owner->CastSpell(owner, 34471, true, NULL, this);
                    else
                        owner->RemoveAurasDueToSpell(34471);
                    break;
                }
            }
        }
    }
    // Heroic Fury (Intercept cooldown remove)
    else if (apply && GetSpellProto()->Id == 60970 && target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->RemoveSpellCooldown(20252, true);
}

void Aura::HandleModMechanicImmunityMask(bool apply, bool /*Real*/)
{
    uint32 mechanic  = m_modifier.m_miscvalue;

    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        GetTarget()->RemoveAurasAtMechanicImmunity(mechanic,GetId());

    // check implemented in Unit::IsImmunedToSpell and Unit::IsImmunedToSpellEffect
}

//this method is called whenever we add / remove aura which gives m_target some imunity to some spell effect
void Aura::HandleAuraModEffectImmunity(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    // when removing flag aura, handle flag drop
    if( !apply && target->GetTypeId() == TYPEID_PLAYER
        && (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION) )
    {
        if( BattleGround *bg = ((Player*)target)->GetBattleGround() )
            bg->EventPlayerDroppedFlag(((Player*)target));
    }

    target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModStateImmunity(bool apply, bool Real)
{
    if(apply && Real && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        Unit::AuraList const& auraList = GetTarget()->GetAurasByType(AuraType(m_modifier.m_miscvalue));
        for(Unit::AuraList::const_iterator itr = auraList.begin(); itr != auraList.end();)
        {
            if (auraList.front() != this)                   // skip itself aura (it already added)
            {
                GetTarget()->RemoveAurasDueToSpell(auraList.front()->GetId());
                itr = auraList.begin();
            }
            else
                ++itr;
        }
    }

    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_STATE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModSchoolImmunity(bool apply, bool Real)
{
    Unit* target = GetTarget();
    target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, m_modifier.m_miscvalue, apply);

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if( GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
        && GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD )
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if( Real && apply
        && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
        && IsPositiveSpell(GetId()) )                       //Only positive immunity removes auras
    {
        uint32 school_mask = m_modifier.m_miscvalue;
        Unit::SpellAuraHolderMap& Auras = target->GetSpellAuraHolderMap();
        for(Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
        {
            next = iter;
            ++next;
            SpellEntry const *spell = iter->second->GetSpellProto();
            if((GetSpellSchoolMask(spell) & school_mask)//Check for school mask
                && !( spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)   //Spells unaffected by invulnerability
                && !iter->second->IsPositive()          //Don't remove positive spells
                && spell->Id != GetId() )               //Don't remove self
            {
                target->RemoveAurasDueToSpell(spell->Id);
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
            target->addUnitState(UNIT_STAT_ISOLATED);
        else
            target->clearUnitState(UNIT_STAT_ISOLATED);
    }
}

void Aura::HandleAuraModDmgImmunity(bool apply, bool /*Real*/)
{
    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModDispelImmunity(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    GetTarget()->ApplySpellDispelImmunity(GetSpellProto(), DispelType(m_modifier.m_miscvalue), apply);
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
                GetHolder()->SetAuraCharges(6);
                break;
            default: break;
        }
    }
}

void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if(apply)
        GetTarget()->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    else
        GetTarget()->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
}

/*********************************************************/
/***                   PERIODIC                        ***/
/*********************************************************/

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit *target = GetTarget();

    if (!apply)
    {
        switch(GetId())
        {
            case 66:                                        // Invisibility
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                    target->CastSpell(target, 32612, true, NULL, this);

                return;
            case 42783:                                     //Wrath of the Astrom...
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE && GetEffIndex() + 1 < MAX_EFFECT_INDEX)
                    target->CastSpell(target, GetSpellProto()->CalculateSimpleValue(SpellEffectIndex(GetEffIndex()+1)), true);
                return;
            case 51912:                                     // Ultra-Advanced Proto-Typical Shortening Blaster
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    if (Unit* pCaster = GetCaster())
                        pCaster->CastSpell(target, GetSpellProto()->EffectTriggerSpell[GetEffIndex()], true, NULL, this);
                }

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

    Unit *target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    if (apply && !loading)
    {
        switch (GetId())
        {
            case 54833:                                     // Glyph of Innervate (value%/2 of casters base mana)
            {
                if (Unit* caster = GetCaster())
                    m_modifier.m_amount = int32(caster->GetCreateMana() * GetBasePoints() / (200 * GetAuraMaxTicks()));
                break;

            }
            case 29166:                                     // Innervate (value% of casters base mana)
            {
                if (Unit* caster = GetCaster())
                {
                    // Glyph of Innervate
                    if (caster->HasAura(54832))
                        caster->CastSpell(caster,54833,true,NULL,this);

                    m_modifier.m_amount = int32(caster->GetCreateMana() * GetBasePoints() / (100 * GetAuraMaxTicks()));
                }
                break;
            }
            case 48391:                                     // Owlkin Frenzy 2% base mana
                m_modifier.m_amount = target->GetCreateMana() * 2 / 100;
                break;
            case 57669:                                     // Replenishment (0.2% from max)
            case 61782:                                     // Infinite Replenishment
                m_modifier.m_amount = target->GetMaxPower(POWER_MANA) * 2 / 1000;
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

    Unit *target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

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
                    case 31666: target->RemoveAurasDueToSpell(31665); break;
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

    Unit *target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        // Gift of the Naaru (have diff spellfamilies)
        if (GetSpellProto()->SpellIconID == 329 && GetSpellProto()->SpellVisual[0] == 7625)
        {
            int32 ap = int32 (0.22f * caster->GetTotalAttackPowerValue(BASE_ATTACK));
            int32 holy = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(GetSpellProto()));
            if  (holy < 0)
                holy = 0;
            holy = int32(holy * 377 / 1000);
            m_modifier.m_amount += ap > holy ? ap : holy;
        }

        m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    m_isPeriodic = apply;

    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        switch (spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                // Pounce Bleed
                if (spellProto->SpellIconID == 147 && spellProto->SpellVisual[0] == 0)
                    // $AP*0.18/6 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 3 / 100);
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                // Rend
                if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000020))
                {
                    // $0.2*(($MWB+$mwb)/2+$AP/14*$MWS) bonus per tick
                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 mws = caster->GetAttackTime(BASE_ATTACK);
                    float mwb_min = caster->GetWeaponDamageRange(BASE_ATTACK,MINDAMAGE);
                    float mwb_max = caster->GetWeaponDamageRange(BASE_ATTACK,MAXDAMAGE);
                    m_modifier.m_amount+=int32(((mwb_min+mwb_max)/2+ap*mws/14000)*0.2f);
                    // If used while target is above 75% health, Rend does 35% more damage
                    if (spellProto->CalculateSimpleValue(EFFECT_INDEX_1) !=0 &&
                        target->GetHealth() > target->GetMaxHealth() * spellProto->CalculateSimpleValue(EFFECT_INDEX_1) / 100)
                        m_modifier.m_amount += m_modifier.m_amount * spellProto->CalculateSimpleValue(EFFECT_INDEX_2) / 100;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                // Rake
                if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000001000) && spellProto->Effect[EFFECT_INDEX_2] == SPELL_EFFECT_ADD_COMBO_POINTS)
                    // $AP*0.18/3 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 6 / 100);
                // Lacerate
                if (spellProto->SpellFamilyFlags & UI64LIT(0x000000010000000000))
                    // $AP*0.05/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                // Rip
                if (spellProto->SpellFamilyFlags & UI64LIT(0x000000000000800000))
                {
                    // 0.01*$AP*cp
                    if (caster->GetTypeId() != TYPEID_PLAYER)
                        break;

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
                }
                // Lock Jaw
                if (spellProto->SpellFamilyFlags & UI64LIT(0x1000000000000000))
                    // 0.15*$AP
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 15 / 100);
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Rupture
                if (spellProto->SpellFamilyFlags & UI64LIT(0x000000000000100000))
                {
                    if (caster->GetTypeId() != TYPEID_PLAYER)
                        break;
                    //1 point : ${($m1+$b1*1+0.015*$AP)*4} damage over 8 secs
                    //2 points: ${($m1+$b1*2+0.024*$AP)*5} damage over 10 secs
                    //3 points: ${($m1+$b1*3+0.03*$AP)*6} damage over 12 secs
                    //4 points: ${($m1+$b1*4+0.03428571*$AP)*7} damage over 14 secs
                    //5 points: ${($m1+$b1*5+0.0375*$AP)*8} damage over 16 secs
                    float AP_per_combo[6] = {0.0f, 0.015f, 0.024f, 0.03f, 0.03428571f, 0.0375f};
                    uint8 cp = ((Player*)caster)->GetComboPoints();
                    if (cp > 5) cp = 5;
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * AP_per_combo[cp]);
                }
                // Garrote
                if (spellProto->SpellFamilyFlags & UI64LIT(0x000000000000000100))
                    // $AP*0.07 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 7 / 100);
                // Deadly Poison
                if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000010000))
                    // 0.12*$AP / 4 * amount of stack
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 3 * GetStackAmount() / 100);
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                // Serpent Sting
                if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000004000))
                    // $RAP*0.2/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 0.2 / 5);
                // Immolation Trap
                if ((spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000004)) && spellProto->SpellIconID == 678)
                    // $RAP*0.1/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 10 / 500);
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                // Holy Vengeance / Blood Corruption
                if (spellProto->SpellFamilyFlags & UI64LIT(0x0000080000000000) && spellProto->SpellVisual[0] == 7902)
                {
                    // AP * 0.025 + SPH * 0.013 bonus per tick
                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto));
                    if (holy < 0)
                        holy = 0;
                    m_modifier.m_amount += int32(GetStackAmount()) * (int32(ap * 0.025f) + int32(holy * 13 / 1000));
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                //Frost Fever and Blood Plague AP scale
                if (spellProto->SpellFamilyFlags & UI64LIT(0x400080000000000))
                {
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.055f * 1.15f);
                }
                break;
            }
            default:
                break;
        }

        if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
        {
            // SpellDamageBonusDone for magic spells
            if(spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
                m_modifier.m_amount = caster->SpellDamageBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
            // MeleeDamagebonusDone for weapon based spells
            else
            {
                WeaponAttackType attackType = GetWeaponAttackType(GetSpellProto());
                m_modifier.m_amount = caster->MeleeDamageBonusDone(target, m_modifier.m_amount, attackType, GetSpellProto(), DOT, GetStackAmount());
            }
        }
    }
    // remove time effects
    else
    {
        // Parasitic Shadowfiend - handle summoning of two Shadowfiends on DoT expire
        if(spellProto->Id == 41917)
            target->CastSpell(target, 41915, true);
    }
}

void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHealthFunnel(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
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
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(m_modifier.m_amount), apply);
            if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                GetTarget()->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
    for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
    {
        if(m_modifier.m_miscvalue & int32(1<<x))
        {
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(GetTarget()->GetTypeId() == TYPEID_PLAYER || ((Creature*)GetTarget())->isPet())
                GetTarget()->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleAuraModBaseResistancePCT(bool apply, bool /*Real*/)
{
    // only players have base stats
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        //pets only have base armor
        if(((Creature*)GetTarget())->isPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(m_modifier.m_amount), apply);
    }
    else
    {
        for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
        {
            if(m_modifier.m_miscvalue & int32(1<<x))
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    for(int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
    {
        if(m_modifier.m_miscvalue & int32(1<<i))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(target->GetTypeId() == TYPEID_PLAYER || ((Creature*)target)->isPet())
            {
                target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, float(m_modifier.m_amount), apply);
                target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, float(m_modifier.m_amount), apply);
            }
        }
    }
}

void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
    // only players have base stats
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        //only pets have base stats
        if(((Creature*)GetTarget())->isPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
    else
    {
        for(int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
            if(m_modifier.m_miscvalue & (1<<i))
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
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
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(GetTarget()->GetTypeId() == TYPEID_PLAYER || ((Creature*)GetTarget())->isPet())
                GetTarget()->ApplyStatBuffMod(Stats(i), float(m_modifier.m_amount), apply);
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
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
    }
}

void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleAuraModDispelResist(bool apply, bool Real)
{
    if(!Real || !apply)
        return;

    if(GetId() == 33206)
        GetTarget()->CastSpell(GetTarget(), 44416, true, NULL, this, GetCasterGUID());
}

void Aura::HandleModSpellDamagePercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    // implemented in Unit::SpellHealingBonusDone
    // this information is for client side only
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    Unit *target = GetTarget();

    //save current and max HP before applying aura
    uint32 curHPValue = target->GetHealth();
    uint32 maxHPValue = target->GetMaxHealth();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(target->GetTypeId() == TYPEID_PLAYER || ((Creature*)target)->isPet())
                target->ApplyStatPercentBuffMod(Stats(i), float(m_modifier.m_amount), apply );
        }
    }

    //recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
    if ((m_modifier.m_miscvalue == STAT_STAMINA) && (maxHPValue > 0) && (GetSpellProto()->Attributes & 0x10))
    {
        // newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
        uint32 newHPValue = (target->GetMaxHealth() * curHPValue) / maxHPValue;
        target->SetHealth(newHPValue);
    }
}

void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if(m_modifier.m_miscvalue != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
        return;
    }

    // Recalculate Armor
    GetTarget()->UpdateArmor();
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

    Powers pt = GetTarget()->getPowerType();
    if(m_modifier.periodictime == 0)
    {
        // Anger Management (only spell use this aura for rage)
        if (pt == POWER_RAGE)
            m_modifier.periodictime = 3000;
        else
            m_modifier.periodictime = 2000;
    }

    m_periodicTimer = 5000;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER && m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)GetTarget())->UpdateManaRegen();

    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Update manaregen value
    if (m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleModManaRegen(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    //Note: an increase in regen does NOT cause threat.
    ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleComprehendLanguage(bool apply, bool /*Real*/)
{
    if(apply)
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    else
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
}

void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // Special case with temporary increase max/current health
    switch(GetId())
    {
        case 12976:                                         // Warrior Last Stand triggered spell
        case 28726:                                         // Nightmare Seed ( Nightmare Seed )
        case 31616:                                         // Nature's Guardian
        case 34511:                                         // Valor (Bulwark of Kings, Bulwark of the Ancient Kings)
        case 44055: case 55915: case 55917: case 67596:     // Tremendous Fortitude (Battlemaster's Alacrity)
        case 50322:                                         // Survival Instincts
        case 54443:                                         // Demonic Empowerment (Voidwalker)
        case 55233:                                         // Vampiric Blood
        case 59465:                                         // Brood Rage (Ahn'Kahet)
        {
            if(Real)
            {
                if(apply)
                {
                    // Demonic Empowerment (Voidwalker) & Vampiric Blood - special cases, store percent in data
                    // recalculate to full amount at apply for proper remove
                    if (GetId() == 54443 || GetId() == 55233)
                        m_modifier.m_amount = target->GetMaxHealth() * m_modifier.m_amount / 100;

                    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                    target->ModifyHealth(m_modifier.m_amount);
                }
                else
                {
                    if (int32(target->GetHealth()) > m_modifier.m_amount)
                        target->ModifyHealth(-m_modifier.m_amount);
                    else
                        target->SetHealth(1);
                    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                }
            }
            return;
        }
    }

    // generic case
    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void  Aura::HandleAuraModIncreaseMaxHealth(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    uint32 oldhealth = target->GetHealth();
    double healthPercentage = (double)oldhealth / (double)target->GetMaxHealth();

    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);

    // refresh percentage
    if(oldhealth > 0)
    {
        uint32 newhealth = uint32(ceil((double)target->GetMaxHealth() * healthPercentage));
        if(newhealth==0)
            newhealth = 1;

        target->SetHealth(newhealth);
    }
}

void Aura::HandleAuraModIncreaseEnergy(bool apply, bool Real)
{
    Unit *target = GetTarget();
    Powers powerType = target->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    // Special case with temporary increase max/current power (percent)
    if (GetId()==64904)                                     // Hymn of Hope
    {
        if(Real)
        {
            uint32 val = target->GetPower(powerType);
            target->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->SetPower(powerType, apply ? val*(100+m_modifier.m_amount)/100 : val*100/(100+m_modifier.m_amount));
        }
        return;
    }

    // generic flat case
    target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
    Powers powerType = GetTarget()->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    GetTarget()->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraIncreaseBaseHealthPercent(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_HEALTH, BASE_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateParryPercentage();
}

void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateDodgePercentage();
    //sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModBlockPercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateBlockPercentage();
    //sLog.outError("BONUS BLOCK CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if(target->GetTypeId() != TYPEID_PLAYER)
        return;

    // apply item specific bonuses for already equipped weapon
    if(Real)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
    }

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if (GetSpellProto()->EquippedItemClass == -1)
    {
        ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (m_modifier.m_amount), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    if(target->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)target)->UpdateMeleeHitChances();
        ((Player*)target)->UpdateRangedHitChances();
    }
    else
    {
        target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
        target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)GetTarget())->UpdateSpellHitChances();
    }
    else
    {
        GetTarget()->m_modSpellHitChance += apply ? m_modifier.m_amount: (-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)GetTarget())->UpdateAllSpellCritChances();
    }
    else
    {
        GetTarget()->m_baseSpellCritChance += apply ? m_modifier.m_amount:(-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellCritChanceShool(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for(int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (m_modifier.m_miscvalue & (1<<school))
            ((Player*)GetTarget())->UpdateSpellCritChance(school);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyCastTimePercentMod(float(m_modifier.m_amount),apply);
}

void Aura::HandleModMeleeRangedSpeedPct(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModCombatSpeedPct(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    target->ApplyCastTimePercentMod(float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyAttackTimePercentMod(BASE_ATTACK,float(m_modifier.m_amount),apply);
}

void Aura::HandleHaste(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
    if((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
    //UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
    if((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    //UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerOfStatPercent(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // Recalculate bonus
    if(GetTarget()->GetTypeId() == TYPEID_PLAYER && !(GetTarget()->getClassMask() & CLASSMASK_WAND_USERS))
        ((Player*)GetTarget())->UpdateAttackPowerAndDamage(true);
}

void Aura::HandleAuraModAttackPowerOfStatPercent(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // Recalculate bonus
    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateAttackPowerAndDamage(false);
}

void Aura::HandleAuraModAttackPowerOfArmor(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // Recalculate bonus
    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateAttackPowerAndDamage(false);
}
/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if(Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
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
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }

        if(target->GetTypeId() == TYPEID_PLAYER)
        {
            if(m_positive)
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
            else
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
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

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    if(target->GetTypeId() == TYPEID_PLAYER)
    {
        if(m_positive)
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
            }
        }
        else
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
            }
        }
        Pet* pet = target->GetPet();
        if(pet)
            pet->UpdateAttackPowerAndDamage();
    }
}

void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if(Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
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
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
        // For show in client
        if(target->GetTypeId() == TYPEID_PLAYER)
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount/100.0f, apply);
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

    // Magic damage percent modifiers implemented in Unit::SpellDamageBonusDone
    // Send info to client
    if(target->GetTypeId() == TYPEID_PLAYER)
        for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount/100.0f, apply);
}

void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD OFFHAND DAMAGE");

    GetTarget()->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
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
            GetTarget()->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void Aura::HandleModPowerCost(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if(m_modifier.m_miscvalue & (1<<i))
            GetTarget()->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
}

void Aura::HandleNoReagentUseAura(bool /*Apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;
    Unit *target = GetTarget();
    if(target->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 mask[3] = {0, 0, 0};
    Unit::AuraList const& noReagent = target->GetAurasByType(SPELL_AURA_NO_REAGENT_USE);
        for(Unit::AuraList::const_iterator i = noReagent.begin(); i !=  noReagent.end(); ++i)
        {
            uint32 const *ptr = (*i)->getAuraSpellClassMask();
            mask[0] |= ptr[0];
            mask[1] |= ptr[1];
            mask[2] |= ptr[2];
        }

    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+0, mask[0]);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+1, mask[1]);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+2, mask[2]);
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

    Unit *target = GetTarget();

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

            if(target->GetTypeId() == TYPEID_PLAYER)      // Spell 49868 have same category as main form spell and share cooldown
                ((Player*)target)->RemoveSpellCooldown(49868);
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
            target->CastSpell(target, spellId1, true, NULL, this );
        if (spellId2)
            target->CastSpell(target, spellId2, true, NULL, this);

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = ((Player *)target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED) continue;
                if (itr->first==spellId1 || itr->first==spellId2) continue;
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !(spellInfo->Attributes & (SPELL_ATTR_PASSIVE | SPELL_ATTR_UNK7)))
                    continue;
                // passive spells with SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT are already active without shapeshift, do no recast!
                if (spellInfo->Stances & (1<<(form-1)) && !(spellInfo->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT))
                    target->CastSpell(target, itr->first, true, NULL, this);
            }
            // remove auras that do not require shapeshift, but are not active in this specific form (like Improved Barkskin)
            Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
            {
                SpellEntry const *spellInfo = itr->second->GetSpellProto();
                if (itr->second->IsPassive() && (spellInfo->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT)
                    && (spellInfo->StancesNot & (1<<(form-1))))
                {
                    target->RemoveAurasDueToSpell(itr->second->GetId());
                    itr = tAuras.begin();
                }
                else
                    ++itr;
            }


            // Master Shapeshifter
            if (MasterShaperSpellId)
            {
                Unit::AuraList const& ShapeShifterAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = ShapeShifterAuras.begin(); i != ShapeShifterAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 2851)
                    {
                        int32 ShiftMod = (*i)->GetModifier()->m_amount;
                        target->CastCustomSpell(target, MasterShaperSpellId, &ShiftMod, NULL, NULL, true);
                        break;
                    }
                }
            }

            // Leader of the Pack
            if (((Player*)target)->HasSpell(17007))
            {
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(24932);
                if (spellInfo && spellInfo->Stances & (1<<(form-1)))
                    target->CastSpell(target, 24932, true, NULL, this);
            }

            // Savage Roar
            if (form == FORM_CAT && ((Player*)target)->HasAura(52610))
                target->CastSpell(target, 62071, true);

            // Survival of the Fittest (Armor part)
            if (form == FORM_BEAR || form == FORM_DIREBEAR)
            {
                Unit::AuraList const& modAuras = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for (Unit::AuraList::const_iterator i = modAuras.begin(); i != modAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID &&
                        (*i)->GetSpellProto()->SpellIconID == 961)
                    {
                        int32 bp = (*i)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2);
                        if (bp)
                            target->CastCustomSpell(target, 62069, &bp, NULL, NULL, true, NULL, this);
                        break;
                    }
                }
            }

            // Improved Moonkin Form
            if (form == FORM_MOONKIN)
            {
                Unit::AuraList const& dummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
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
                            target->CastSpell(target, spell_id, true, NULL, this);
                        break;
                    }
                }
            }

            // Heart of the Wild
            if (HotWSpellId)
            {
                Unit::AuraList const& mModTotalStatPct = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for(Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
                    {
                        int32 HotWMod = (*i)->GetModifier()->m_amount;
                        if(GetModifier()->m_miscvalue == FORM_CAT)
                            HotWMod /= 2;

                        target->CastCustomSpell(target, HotWSpellId, &HotWMod, NULL, NULL, true, NULL, this);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if(spellId1)
            target->RemoveAurasDueToSpell(spellId1);
        if(spellId2)
            target->RemoveAurasDueToSpell(spellId2);
        if(MasterShaperSpellId)
            target->RemoveAurasDueToSpell(MasterShaperSpellId);

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            // re-apply passive spells that don't need shapeshift but were inactive in current form:
            const PlayerSpellMap& sp_list = ((Player *)target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED) continue;
                if (itr->first==spellId1 || itr->first==spellId2) continue;
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !IsPassiveSpell(spellInfo))
                    continue;
                if ((spellInfo->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) && spellInfo->StancesNot & (1<<(form-1)))
                    target->CastSpell(target, itr->first, true, NULL, this);
            }
        }

        Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->IsRemovedOnShapeLost())
            {
                target->RemoveAurasDueToSpell(itr->second->GetId());
                itr = tAuras.begin();
            }
            else
                ++itr;
        }
    }
}

void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_UNIT)
        return;

    CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(GetTarget()->GetEntry());
    if(ci && ci->type == CREATURE_TYPE_BEAST)
        GetTarget()->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
    if(apply)
        GetTarget()->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    else
        GetTarget()->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
}

void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if(apply)
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
    HandleAuraModPacify(apply, Real);
    HandleAuraModSilence(apply, Real);
}

void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if(apply)
    {
        GetTarget()->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
    else
    {
        GetTarget()->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
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
    data << GetTarget()->GetPackGUID();
    data << uint32(0);                                      // unk
    GetTarget()->SendMessageToSet(&data, true);
}

void Aura::HandleModRating(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (m_modifier.m_miscvalue & (1 << rating))
            ((Player*)GetTarget())->ApplyRatingMod(CombatRating(rating), m_modifier.m_amount, apply);
}

void Aura::HandleModRatingFromStat(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    // Just recalculate ratings
    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (m_modifier.m_miscvalue & (1 << rating))
            ((Player*)GetTarget())->ApplyRatingMod(CombatRating(rating), 0, apply);
}

void Aura::HandleForceMoveForward(bool apply, bool Real)
{
    if(!Real || GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    if(apply)
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
    else
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
}

void Aura::HandleAuraModExpertise(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateExpertise(BASE_ATTACK);
    ((Player*)GetTarget())->UpdateExpertise(OFF_ATTACK);
}

void Aura::HandleModTargetResistance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;
    Unit *target = GetTarget();
    // applied to damage as HandleNoImmediateEffect in Unit::CalculateAbsorbAndResist and Unit::CalcArmorReducedDamage
    // show armor penetration
    if (target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, m_modifier.m_amount, apply);

    // show as spell penetration only full spell penetration bonuses (all resistances except armor and holy
    if (target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_SPELL)==SPELL_SCHOOL_MASK_SPELL)
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, m_modifier.m_amount, apply);
}

void Aura::HandleShieldBlockValue(bool apply, bool /*Real*/)
{
    BaseModType modType = FLAT_MOD;
    if(m_modifier.m_auraname == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *target = (Player*)GetTarget();

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if( !apply && m_duration==0 && target->GetComboTarget())
        if(Unit* unit = ObjectAccessor::GetUnit(*GetTarget(),target->GetComboTarget()))
            target->AddComboPoints(unit, -m_modifier.m_amount);
}

void Aura::HandleModUnattackable( bool Apply, bool Real )
{
    if(Real && Apply)
     {
        GetTarget()->CombatStop();
        GetTarget()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
     }
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE,Apply);
}

void Aura::HandleSpiritOfRedemption( bool apply, bool Real )
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    // prepare spirit state
    if(apply)
    {
        if(target->GetTypeId()==TYPEID_PLAYER)
        {
            // disable breath/etc timers
            ((Player*)target)->StopMirrorTimers();

            // set stand state (expected in this form)
            if(!target->IsStandState())
                target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        target->SetHealth(1);
    }
    // die at aura end
    else
        target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
}

void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();
    if (apply)
    {
        // prevent double apply bonuses
        if (target->GetTypeId()!=TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
        {
            float DoneActualBenefit = 0.0f;
            switch(spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_PRIEST:
                    // Power Word: Shield
                    if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001))
                    {
                        //+80.68% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto)) * 0.8068f;
                        //Borrowed Time
                        Unit::AuraList const& borrowedTime = caster->GetAurasByType(SPELL_AURA_DUMMY);
                        for(Unit::AuraList::const_iterator itr = borrowedTime.begin(); itr != borrowedTime.end(); ++itr)
                        {
                            SpellEntry const* i_spell = (*itr)->GetSpellProto();
                            if(i_spell->SpellFamilyName==SPELLFAMILY_PRIEST && i_spell->SpellIconID == 2899 && i_spell->EffectMiscValue[(*itr)->GetEffIndex()] == 24)
                            {
                                DoneActualBenefit += DoneActualBenefit * (*itr)->GetModifier()->m_amount / 100;
                                break;
                            }
                        }
                    }

                    break;
                case SPELLFAMILY_MAGE:
                    // Frost Ward, Fire Ward
                    if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000108))
                        //+10% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
                    // Ice Barrier
                    else if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000100000000))
                        //+80.67% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.8067f;
                    break;
                case SPELLFAMILY_WARLOCK:
                    // Shadow Ward
                    if (spellProto->SpellFamilyFlags2 & 0x00000040)
                        //+30% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.30f;
                    break;
                case SPELLFAMILY_PALADIN:
                    // Sacred Shield
                    // (check not strictly needed, only Sacred Shield has SPELL_AURA_SCHOOL_ABSORB in SPELLFAMILY_PALADIN at this time)
                    if (spellProto->SpellFamilyFlags & UI64LIT(0x0008000000000000))
                    {
                        // +75% from spell power
                        DoneActualBenefit = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto)) * 0.75f;
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
            spellProto->SpellFamilyName == SPELLFAMILY_PRIEST && spellProto->Mechanic == MECHANIC_SHIELD &&
            (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001)) &&
            // completely absorbed or dispelled
            (m_removeMode == AURA_REMOVE_BY_SHIELD_BREAK || m_removeMode == AURA_REMOVE_BY_DISPEL))
        {
            Unit::AuraList const& vDummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
            for(Unit::AuraList::const_iterator itr = vDummyAuras.begin(); itr != vDummyAuras.end(); ++itr)
            {
                SpellEntry const* vSpell = (*itr)->GetSpellProto();

                // Rapture (main spell)
                if(vSpell->SpellFamilyName == SPELLFAMILY_PRIEST && vSpell->SpellIconID == 2894 && vSpell->Effect[EFFECT_INDEX_1])
                {
                    switch((*itr)->GetEffIndex())
                    {
                        case EFFECT_INDEX_0:
                        {
                            // energize caster
                            int32 manapct1000 = 5 * ((*itr)->GetModifier()->m_amount + sSpellMgr.GetSpellRank(vSpell->Id));
                            int32 basepoints0 = caster->GetMaxPower(POWER_MANA) * manapct1000 / 1000;
                            caster->CastCustomSpell(caster, 47755, &basepoints0, NULL, NULL, true);
                            break;
                        }
                        case EFFECT_INDEX_1:
                        {
                            // energize target
                            if (!roll_chance_i((*itr)->GetModifier()->m_amount) || caster->HasAura(63853))
                                break;

                            switch(target->getPowerType())
                            {
                                case POWER_RUNIC_POWER:
                                    target->CastSpell(target, 63652, true, NULL, NULL, GetCasterGUID());
                                    break;
                                case POWER_RAGE:
                                    target->CastSpell(target, 63653, true, NULL, NULL, GetCasterGUID());
                                    break;
                                case POWER_MANA:
                                    {
                                        int32 basepoints0 = target->GetMaxPower(POWER_MANA) * 2 / 100;
                                        target->CastCustomSpell(target, 63654, &basepoints0, NULL, NULL, true);
                                        break;
                                    }
                                case POWER_ENERGY:
                                    target->CastSpell(target, 63655, true, NULL, NULL, GetCasterGUID());
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
    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    switch(m_modifier.m_auraname)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if( spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(target, spellProto, false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if(target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
                return;

            // some auras remove at specific health level or more
            if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
            {
                switch(GetId())
                {
                    case 43093: case 31956: case 38801:
                    case 35321: case 38363: case 39215:
                    case 48920:
                    {
                        if(target->GetHealth() == target->GetMaxHealth() )
                        {
                            target->RemoveAurasDueToSpell(GetId());
                            return;
                        }
                        break;
                    }
                    case 38772:
                    {
                        uint32 percent =
                            GetEffIndex() < EFFECT_INDEX_2 && spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_DUMMY ?
                            pCaster->CalculateSpellDamage(target, spellProto, SpellEffectIndex(GetEffIndex() + 1)) :
                            100;
                        if(target->GetHealth() * 100 >= target->GetMaxHealth() * percent )
                        {
                            target->RemoveAurasDueToSpell(GetId());
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
                pdamage = amount;
            else
                pdamage = uint32(target->GetMaxHealth()*amount/100);


            // SpellDamageBonus for magic spells
            if(spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
                pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());
            // MeleeDamagebonus for weapon based spells
            else
            {
                WeaponAttackType attackType = GetWeaponAttackType(spellProto);
                pdamage = target->MeleeDamageBonusTaken(pCaster, pdamage, attackType, spellProto, DOT, GetStackAmount());
            }

            // Calculate armor mitigation if it is a physical spell
            // But not for bleed mechanic spells
            if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL &&
                GetEffectMechanic(spellProto, m_effIndex) != MECHANIC_BLEED)
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            // Curse of Agony damage-per-tick calculation
            if (spellProto->SpellFamilyName==SPELLFAMILY_WARLOCK && (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000400)) && spellProto->SpellIconID==544)
            {
                // 1..4 ticks, 1/2 from normal tick damage
                if (GetAuraTicks() <= 4)
                    pdamage = pdamage/2;
                // 9..12 ticks, 3/2 from normal tick damage
                else if(GetAuraTicks() >= 9)
                    pdamage += (pdamage + 1) / 2;       // +1 prevent 0.5 damage possible lost at 1..4 ticks
                // 5..8 ticks have normal tick damage
            }

            // This method can modify pdamage
            bool isCrit = IsCritFromAbilityAura(pCaster, pdamage);

            // send critical in hit info for threat calculation
            if (isCrit)
            {
                cleanDamage.hitOutCome = MELEE_HIT_CRIT;
                // Resilience - reduce crit damage
                pdamage -= target->GetSpellCritDamageReduction(pdamage);
            }

            // only from players
            // FIXME: need use SpellDamageBonus instead?
            if (pCaster->GetTypeId() == TYPEID_PLAYER)
                pdamage -= target->GetSpellDamageReduction(pdamage);

            target->CalculateAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, !(GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %u (TypeId: %u) attacked %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), pdamage, GetId(),absorb);

            pCaster->DealDamageMods(target, pdamage, &absorb);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
            pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, absorb, resist, 0.0f, isCrit);
            target->SendPeriodicAuraLog(&pInfo);

            if (pdamage)
                procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;

            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);

            pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, true);

            // Drain Soul (chance soul shard)
            if (pCaster->GetTypeId() == TYPEID_PLAYER && spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && spellProto->SpellFamilyFlags & UI64LIT(0x0000000000004000))
            {
                // Only from non-grey units
                if (roll_chance_i(10) &&                    // 1-2 from drain with final and without glyph, 0-1 from damage
                    ((Player*)pCaster)->isHonorOrXPTarget(target) &&
                    (target->GetTypeId() != TYPEID_UNIT || ((Player*)pCaster)->isAllowedToLoot((Creature*)target)))
                {
                    pCaster->CastSpell(pCaster, 43836, true, NULL, this);
                }
            }

            break;
        }
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if(!pCaster->isAlive())
                return;

            if( spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(target, spellProto, false) != SPELL_MISS_NONE)
                return;

            // Check for immune
            if(target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
                return;

            uint32 absorb=0;
            uint32 resist=0;
            CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );

            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            //Calculate armor mitigation if it is a physical spell
            if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL)
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

            bool isCrit = IsCritFromAbilityAura(pCaster, pdamage);

            // send critical in hit info for threat calculation
            if (isCrit)
            {
                cleanDamage.hitOutCome = MELEE_HIT_CRIT;
                // Resilience - reduce crit damage
                pdamage -= target->GetSpellCritDamageReduction(pdamage);
            }

            // only from players
            // FIXME: need use SpellDamageBonus instead?
            if (IS_PLAYER_GUID(GetCasterGUID()))
                pdamage -= target->GetSpellDamageReduction(pdamage);

            target->CalculateAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, !(spellProto->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));

            if(target->GetHealth() < pdamage)
                pdamage = uint32(target->GetHealth());

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %u (TypeId: %u) health leech of %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), pdamage, GetId(),absorb);

            pCaster->DealDamageMods(target, pdamage, &absorb);

            pCaster->SendSpellNonMeleeDamageLog(target, GetId(), pdamage, GetSpellSchoolMask(spellProto), absorb, resist, false, 0, isCrit);

            float multiplier = spellProto->EffectMultipleValue[GetEffIndex()] > 0 ? spellProto->EffectMultipleValue[GetEffIndex()] : 1;

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
            pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage-absorb-resist);
            if (pdamage)
                procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;
            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);
            int32 new_damage = pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, false);

            if (!target->isAlive() && pCaster->IsNonMeleeSpellCasted(false))
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                    if (Spell* spell = pCaster->GetCurrentSpell(CurrentSpellTypes(i)))
                        if (spell->m_spellInfo->Id == GetId())
                            spell->cancel();


            if(Player *modOwner = pCaster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, multiplier);

            int32 heal = pCaster->SpellHealingBonusTaken(pCaster, spellProto, int32(new_damage * multiplier), DOT, GetStackAmount());

            int32 gain = pCaster->DealHeal(pCaster, heal, spellProto);
            pCaster->getHostileRefManager().threatAssist(pCaster, gain * 0.5f, spellProto);
            break;
        }
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        {
            // don't heal target if not alive, mostly death persistent effects from items
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            // heal for caster damage (must be alive)
            if(target != pCaster && spellProto->SpellVisual[0] == 163 && !pCaster->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage;

            if(m_modifier.m_auraname==SPELL_AURA_OBS_MOD_HEALTH)
                pdamage = uint32(target->GetMaxHealth() * amount / 100);
            else
            {
                pdamage = amount;

                // Wild Growth (1/7 - 6 + 2*ramainTicks) %
                if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && spellProto->SpellIconID == 2864)
                {
                    int32 ticks = GetAuraMaxTicks();
                    int32 remainingTicks = ticks - GetAuraTicks();
                    int32 addition = int32(amount)*ticks*(-6+2*remainingTicks)/100;

                    if (GetAuraTicks() != 1)
                        // Item - Druid T10 Restoration 2P Bonus
                        if (Aura *aura = pCaster->GetAura(70658, EFFECT_INDEX_0))
                            addition += abs(int32((addition * aura->GetModifier()->m_amount) / ((ticks-1)* 100)));

                    pdamage = int32(pdamage) + addition;
                }
            }

            pdamage = target->SpellHealingBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

            // This method can modify pdamage
            bool isCrit = IsCritFromAbilityAura(pCaster, pdamage);

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %u (TypeId: %u) heal of %u (TypeId: %u) for %u health inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), pdamage, GetId());

            int32 gain = target->ModifyHealth(pdamage);
            SpellPeriodicAuraLogInfo pInfo(this, pdamage, (pdamage - uint32(gain)), 0, 0, 0.0f, isCrit);
            target->SendPeriodicAuraLog(&pInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;
            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, gain, BASE_ATTACK, spellProto);

            // add HoTs to amount healed in bgs
            if( pCaster->GetTypeId() == TYPEID_PLAYER )
                if( BattleGround *bg = ((Player*)pCaster)->GetBattleGround() )
                    bg->UpdatePlayerScore(((Player*)pCaster), SCORE_HEALING_DONE, gain);

            target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, spellProto);

            // heal for caster damage
            if(target != pCaster && spellProto->SpellVisual[0] == 163)
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
                    pCaster->SendSpellNonMeleeDamageLog(pCaster, GetId(), damage, GetSpellSchoolMask(spellProto), absorb, 0, false, 0, false);

                    CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );
                    pCaster->DealDamage(pCaster, damage, &cleanDamage, NODAMAGE, GetSpellSchoolMask(spellProto), spellProto, true);
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
            // don't damage target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                return;

            Powers power = Powers(m_modifier.m_miscvalue);

            // power type might have changed between aura applying and tick (druid's shapeshift)
            if(target->getPowerType() != power)
                return;

            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if(!pCaster->isAlive())
                return;

            if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(target, spellProto, false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if(target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            // Special case: draining x% of mana (up to a maximum of 2*x% of the caster's maximum mana)
            // It's mana percent cost spells, m_modifier.m_amount is percent drain from target
            if (spellProto->ManaCostPercentage)
            {
                // max value
                uint32 maxmana = pCaster->GetMaxPower(power)  * pdamage * 2 / 100;
                pdamage = target->GetMaxPower(power) * pdamage / 100;
                if(pdamage > maxmana)
                    pdamage = maxmana;
            }

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %u (TypeId: %u) power leech of %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), pdamage, GetId());

            int32 drain_amount = target->GetPower(power) > pdamage ? pdamage : target->GetPower(power);

            // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
            if (power == POWER_MANA)
                drain_amount -= target->GetSpellCritDamageReduction(drain_amount);

            target->ModifyPower(power, -drain_amount);

            float gain_multiplier = 0;

            if(pCaster->GetMaxPower(power) > 0)
            {
                gain_multiplier = spellProto->EffectMultipleValue[GetEffIndex()];

                if(Player *modOwner = pCaster->GetSpellModOwner())
                    modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, gain_multiplier);
            }

            SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, 0, gain_multiplier);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain_amount = int32(drain_amount * gain_multiplier);

            if(gain_amount)
            {
                int32 gain = pCaster->ModifyPower(power, gain_amount);
                target->AddThreat(pCaster, float(gain) * 0.5f, pInfo.critical, GetSpellSchoolMask(spellProto), spellProto);
            }
            break;
        }
        case SPELL_AURA_PERIODIC_ENERGIZE:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), pdamage, GetId());

            if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                break;

            Powers power = Powers(m_modifier.m_miscvalue);

            if(target->GetMaxPower(power) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain = target->ModifyPower(power,pdamage);

            if(Unit* pCaster = GetCaster())
                target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, spellProto);
            break;
        }
        case SPELL_AURA_OBS_MOD_MANA:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage = uint32(target->GetMaxPower(POWER_MANA) * amount / 100);

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u mana inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), pdamage, GetId());

            if(target->GetMaxPower(POWER_MANA) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain = target->ModifyPower(POWER_MANA, pdamage);

            if(Unit* pCaster = GetCaster())
                target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, spellProto);
            break;
        }
        case SPELL_AURA_POWER_BURN_MANA:
        {
            // don't mana burn target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            // Check for immune (not use charges)
            if(target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
                return;

            int32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            Powers powerType = Powers(m_modifier.m_miscvalue);

            if(!target->isAlive() || target->getPowerType() != powerType)
                return;

            // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
            if (powerType == POWER_MANA)
                pdamage -= target->GetSpellCritDamageReduction(pdamage);

            uint32 gain = uint32(-target->ModifyPower(powerType, -pdamage));

            gain = uint32(gain * spellProto->EffectMultipleValue[GetEffIndex()]);

            // maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
            SpellNonMeleeDamage damageInfo(pCaster, target, spellProto->Id, SpellSchoolMask(spellProto->SchoolMask));
            pCaster->CalculateSpellDamage(&damageInfo, gain, spellProto);

            damageInfo.target->CalculateAbsorbResistBlock(pCaster, &damageInfo, spellProto);

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
            // don't heal target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            int32 gain = target->ModifyHealth(m_modifier.m_amount);
            if (Unit *caster = GetCaster())
                target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, spellProto);
            break;
        }
        case SPELL_AURA_MOD_POWER_REGEN:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Powers pt = target->getPowerType();
            if(int32(pt) != m_modifier.m_miscvalue)
                return;

            if ( spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED )
            {
                // eating anim
                target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
            }
            else if( GetId() == 20577 )
            {
                // cannibalize anim
                target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
            }

            // Anger Management
            // amount = 1+ 16 = 17 = 3,4*5 = 10,2*5/3
            // so 17 is rounded amount for 5 sec tick grow ~ 1 range grow in 3 sec
            if(pt == POWER_RAGE)
                target->ModifyPower(pt, m_modifier.m_amount * 3 / 5);
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
    Unit *target = GetTarget();
    switch (spell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (spell->Id)
            {
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
                        target->CastSpell(target,m_modifier.m_amount,true,NULL,this);
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
                case 46041:                                 // Summon Blood Elves Periodic
                    target->CastSpell(target, 46037, true, NULL, this);
                    target->CastSpell(target, roll_chance_i(50) ? 46038 : 46039, true, NULL, this);
                    target->CastSpell(target, 46040, true, NULL, this);
                    return;
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
                case 48630:                                 // Summon Gauntlet Mobs Periodic
                case 59275:                                 // Below may need some adjustment, pattern for amount of summon and where is not verified 100% (except for odd/even tick)
                {
                    bool chance = roll_chance_i(50);

                    target->CastSpell(target, chance ? 48631 : 48632, true, NULL, this);

                    if (GetAuraTicks() % 2)                 // which doctor at odd tick
                        target->CastSpell(target, chance ? 48636 : 48635, true, NULL, this);
                    else                                    // or harponeer, at even tick
                        target->CastSpell(target, chance ? 48634 : 48633, true, NULL, this);

                    return;
                }
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
                case 50789:                                 // Summon iron dwarf (left or right)
                case 59860:
                    target->CastSpell(target, roll_chance_i(50) ? 50790 : 50791, true, NULL, this);
                    return;
                case 50792:                                 // Summon iron trogg (left or right)
                case 59859:
                    target->CastSpell(target, roll_chance_i(50) ? 50793 : 50794, true, NULL, this);
                    return;
                case 50801:                                 // Summon malformed ooze (left or right)
                case 59858:
                    target->CastSpell(target, roll_chance_i(50) ? 50802 : 50803, true, NULL, this);
                    return;
                case 50824:                                 // Summon earthen dwarf
                    target->CastSpell(target, roll_chance_i(50) ? 50825 : 50826, true, NULL, this);
                    return;
                case 52441:                                 // Cool Down
                    target->CastSpell(target, 52443, true);
                    return;
                case 53520:                                 // Carrion Beetles
                    target->CastSpell(target, 53521, true, NULL, this);
                    target->CastSpell(target, 53521, true, NULL, this);
                    return;
                case 55592:                                 // Clean
                    switch(urand(0,2))
                    {
                        case 0: target->CastSpell(target, 55731, true); break;
                        case 1: target->CastSpell(target, 55738, true); break;
                        case 2: target->CastSpell(target, 55739, true); break;
                    }
                    return;
// Exist more after, need add later
                default:
                    break;
            }

            // Drink (item drink spells)
            if (GetEffIndex() > EFFECT_INDEX_0 && spell->EffectApplyAuraName[GetEffIndex()-1] == SPELL_AURA_MOD_POWER_REGEN)
            {
                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;
                // Search SPELL_AURA_MOD_POWER_REGEN aura for this spell and add bonus
                if (Aura* aura = GetHolder()->GetAuraByEffectIndex(SpellEffectIndex(GetEffIndex() - 1)))
                {
                    aura->GetModifier()->m_amount = m_modifier.m_amount;
                    ((Player*)target)->UpdateManaRegen();
                    // Disable continue
                    m_isPeriodic = false;
                    return;
                }
                return;
            }

            // Prey on the Weak
            if (spell->SpellIconID == 2983)
            {
                Unit *victim = target->getVictim();
                if (victim && (target->GetHealth() * 100 / target->GetMaxHealth() > victim->GetHealth() * 100 / victim->GetMaxHealth()))
                {
                    if(!target->HasAura(58670))
                    {
                        int32 basepoints = GetBasePoints();
                        target->CastCustomSpell(target, 58670, &basepoints, 0, 0, true);
                    }
                }
                else
                    target->RemoveAurasDueToSpell(58670);
            }
            break;
        }
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
                    if (target->getPowerType() != POWER_RAGE)
                        return;
                    uint32 rage = target->GetPower(POWER_RAGE);
                    // Nothing todo
                    if (rage == 0)
                        return;
                    int32 mod = (rage < 100) ? rage : 100;
                    int32 points = target->CalculateSpellDamage(target, spell, EFFECT_INDEX_1);
                    int32 regen = target->GetMaxHealth() * (mod * points / 10) / 1000;
                    target->CastCustomSpell(target, 22845, &regen, NULL, NULL, true, NULL, this);
                    target->SetPower(POWER_RAGE, rage-mod);
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
                    if (target->hasUnitState(UNIT_STAT_STUNNED) || target->isFeared())
                        return;

                    std::list<Unit*> targets;
                    {
                        // eff_radius ==0
                        float radius = GetSpellMaxRange(sSpellRangeStore.LookupEntry(spell->rangeIndex));

                        MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck u_check(target, target, radius);
                        MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck> checker(target, targets, u_check);
                        Cell::VisitAllObjects(target, checker, radius);
                    }

                    if(targets.empty())
                        return;

                    std::list<Unit*>::const_iterator itr = targets.begin();
                    std::advance(itr, rand()%targets.size());
                    Unit* victim = *itr;

                    target->CastSpell(victim, 57840, true);
                    target->CastSpell(victim, 57841, true);
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
                target->CastCustomSpell(target, 53352, &m_modifier.m_amount, 0, 0, true, 0, this, GetCasterGUID());
                return;
            }
            switch (spell->Id)
            {
                // Harpooner's Mark
                // case 40084:
                //    return;
                // Feeding Frenzy Rank 1 & 2
                case 53511:
                case 53512:
                {
                    Unit* victim = target->getVictim();
                    if( victim && victim->GetHealth() * 100 < victim->GetMaxHealth() * 35 )
                        target->CastSpell(target, spell->Id == 53511 ? 60096 : 60097, true, NULL, this);
                    return;
                }
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
                if (!(target->GetUInt32Value(UNIT_FIELD_FLAGS)&(UNIT_FLAG_STUNNED|UNIT_FLAG_FLEEING|UNIT_FLAG_SILENCED)))
                    target->RemoveAurasDueToSpell(52179);
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
                    caster->CastCustomSpell(target, 52212, &m_modifier.m_amount, NULL, NULL, true, NULL, this);
                return;
            }
            // Raise Dead
//            if (spell->SpellFamilyFlags & UI64LIT(0x0000000000001000))
//                return;
            // Chains of Ice
            if (spell->SpellFamilyFlags & UI64LIT(0x0000400000000000))
            {
                // Get 0 effect aura
                Aura *slow = target->GetAura(GetId(), EFFECT_INDEX_0);
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
                int32 apBonus = m_modifier.m_amount * target->GetArmor() / target->CalculateSpellDamage(target, spell, EFFECT_INDEX_1);
                target->CastCustomSpell(target, 61217, &apBonus, &apBonus, NULL, true, NULL, this);
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

    Unit::AuraList const& fearAuras = GetTarget()->GetAurasByType(SPELL_AURA_MOD_FEAR);
    if( !fearAuras.empty() )
    {
        if (apply)
            GetTarget()->SetFeared(false, fearAuras.front()->GetCasterGUID());
        else
            GetTarget()->SetFeared(true);
    }
}

void Aura::HandleManaShield(bool apply, bool Real)
{
    if(!Real)
        return;

    // prevent double apply bonuses
    if(apply && (GetTarget()->GetTypeId()!=TYPEID_PLAYER || !((Player*)GetTarget())->GetSession()->PlayerLoading()))
    {
        if(Unit* caster = GetCaster())
        {
            float DoneActualBenefit = 0.0f;
            switch(GetSpellProto()->SpellFamilyName)
            {
                case SPELLFAMILY_MAGE:
                    if(GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000008000))
                    {
                        // Mana Shield
                        // +50% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(GetSpellProto())) * 0.5f;
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
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
    else
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
}

/**
 * Such auras are applied from a caster(=player) to a vehicle.
 * This has been verified using spell #49256
 */
void Aura::HandleAuraControlVehicle(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* target = GetTarget();
    if (target->GetTypeId() != TYPEID_UNIT || !((Creature*)target)->isVehicle())
        return;
    Vehicle* vehicle = (Vehicle*)target;

    Unit *player = GetCaster();
    if(!player || player->GetTypeId() != TYPEID_PLAYER)
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

void Aura::HandleAuraOpenStable(bool apply, bool Real)
{
    if(!Real || GetTarget()->GetTypeId() != TYPEID_PLAYER || !GetTarget()->IsInWorld())
        return;

    Player* player = (Player*)GetTarget();

    if (apply)
        player->GetSession()->SendStablePet(player->GetObjectGuid());

    // client auto close stable dialog at !apply aura
}

void Aura::HandleAuraConvertRune(bool apply, bool Real)
{
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *plr = (Player*)GetTarget();

    if(plr->getClass() != CLASS_DEATH_KNIGHT)
        return;

    RuneType runeFrom = RuneType(GetSpellProto()->EffectMiscValue[m_effIndex]);
    RuneType runeTo   = RuneType(GetSpellProto()->EffectMiscValueB[m_effIndex]);

    if (apply)
    {
        for(uint32 i = 0; i < MAX_RUNES; ++i)
        {
            if (plr->GetCurrentRune(i) == runeFrom && !plr->GetRuneCooldown(i))
            {
                plr->ConvertRune(i, runeTo);
                break;
            }
        }
    }
    else
    {
        for(uint32 i = 0; i < MAX_RUNES; ++i)
        {
            if(plr->GetCurrentRune(i) == runeTo && plr->GetBaseRune(i) == runeFrom)
            {
                plr->ConvertRune(i, runeFrom);
                break;
            }
        }
    }
}

void Aura::HandlePhase(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    // always non stackable
    if(apply)
    {
        Unit::AuraList const& phases = target->GetAurasByType(SPELL_AURA_PHASE);
        if(!phases.empty())
            target->RemoveAurasDueToSpell(phases.front()->GetId(), GetHolder());
    }

    // no-phase is also phase state so same code for apply and remove

    // phase auras normally not expected at BG but anyway better check
    if(target->GetTypeId() == TYPEID_PLAYER)
    {
        // drop flag at invisible in bg
        if(((Player*)target)->InBattleGround())
            if(BattleGround *bg = ((Player*)target)->GetBattleGround())
                bg->EventPlayerDroppedFlag((Player*)target);

        // GM-mode have mask 0xFFFFFFFF
        if(!((Player*)target)->isGameMaster())
            target->SetPhaseMask(apply ? GetMiscValue() : PHASEMASK_NORMAL, false);

        ((Player*)target)->GetSession()->SendSetPhaseShift(apply ? GetMiscValue() : PHASEMASK_NORMAL);

        if (GetEffIndex() == EFFECT_INDEX_0)
        {
            SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
            if(saBounds.first != saBounds.second)
            {
                uint32 zone, area;
                target->GetZoneAndAreaId(zone, area);

                for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
                {
                    // some auras remove at aura remove
                    if(!itr->second->IsFitToRequirements((Player*)target, zone, area))
                        target->RemoveAurasDueToSpell(itr->second->spellId);
                    // some auras applied at aura apply
                    else if(itr->second->autocast)
                    {
                        if (!target->HasAura(itr->second->spellId, EFFECT_INDEX_0))
                            target->CastSpell(target, itr->second->spellId, true);
                    }
                }
            }
        }
    }
    else
        target->SetPhaseMask(apply ? GetMiscValue() : PHASEMASK_NORMAL, false);

    // need triggering visibility update base at phase update of not GM invisible (other GMs anyway see in any phases)
    if(target->GetVisibility() != VISIBILITY_OFF)
        target->SetVisibility(target->GetVisibility());
}

void Aura::HandleAuraSafeFall( bool Apply, bool Real )
{
    // implemented in WorldSession::HandleMovementOpcodes

    // only special case
    if(Apply && Real && GetId() == 32474 && GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->ActivateTaxiPathTo(506, GetId());
}

bool Aura::IsCritFromAbilityAura(Unit* caster, uint32& damage)
{
    Unit::AuraList const& auras = caster->GetAurasByType(SPELL_AURA_ABILITY_PERIODIC_CRIT);
    for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        if (!(*itr)->isAffectedOnSpell(GetSpellProto()))
            continue;
        if (!caster->IsSpellCrit(GetTarget(), GetSpellProto(), GetSpellSchoolMask(GetSpellProto())))
            break;

        damage = caster->SpellCriticalDamageBonus(GetSpellProto(), damage, GetTarget());
        return true;
    }
    return false;
}

void Aura::HandleModTargetArmorPct(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateArmorPenetration();
}

void Aura::HandleAuraModAllCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (m_modifier.m_amount), apply);
    ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (m_modifier.m_amount), apply);
    ((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (m_modifier.m_amount), apply);

    // included in Player::UpdateSpellCritChance calculation
    ((Player*)target)->UpdateAllSpellCritChances();
}

void Aura::HandleAllowOnlyAbility(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(apply)
    {
        target->setAttackTimer(BASE_ATTACK,m_duration);
        target->setAttackTimer(RANGED_ATTACK,m_duration);
        target->setAttackTimer(OFF_ATTACK,m_duration);
    }
    else
    {
        target->resetAttackTimer(BASE_ATTACK);
        target->resetAttackTimer(RANGED_ATTACK);
        target->resetAttackTimer(OFF_ATTACK);
    }

    target->UpdateDamagePhysical(BASE_ATTACK);
    target->UpdateDamagePhysical(RANGED_ATTACK);
    target->UpdateDamagePhysical(OFF_ATTACK);
}

void Aura::SetAuraMaxDuration( int32 duration )
{
	m_maxduration = duration;

	// possible overwrite persistent state
	if (duration > 0)
	{
		if (!(GetHolder()->IsPassive() && GetSpellProto()->DurationIndex == 0))
			GetHolder()->SetPermanent(false);

		GetHolder()->SetAuraFlags(GetHolder()->GetAuraFlags() | AFLAG_DURATION);
	}
}

bool Aura::IsLastAuraOnHolder()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (i != GetEffIndex() && GetHolder()->m_auras[i])
            return false;
    return true;
}

SpellAuraHolder::SpellAuraHolder(SpellEntry const* spellproto, Unit *target, WorldObject *caster, Item *castItem) : m_caster_guid(0), m_target(target),
m_castItemGuid(castItem?castItem->GetGUID():0), m_permanent(false),
m_isRemovedOnShapeLost(true), m_in_use(0), m_deleted(false), m_removeMode(AURA_REMOVE_BY_DEFAULT), m_AuraDRGroup(DIMINISHING_NONE), m_auraSlot(MAX_AURAS),
m_auraFlags(AFLAG_NONE), m_auraLevel(1), m_procCharges(0), m_stackAmount(1)
{
    MANGOS_ASSERT(target);
    MANGOS_ASSERT(spellproto && spellproto == sSpellStore.LookupEntry( spellproto->Id ) && "`info` must be pointer to sSpellStore element");

    if(!caster)
        m_caster_guid = target->GetGUID();
    else
    {
        // remove this assert when not unit casters will be supported
        MANGOS_ASSERT(caster->GetObjectGuid().IsUnit())
        m_caster_guid = caster->GetGUID();
    }

    m_applyTime = time(NULL);
    m_spellProto = spellproto;
    m_isPassive = IsPassiveSpell(GetId());
    m_isDeathPersist = IsDeathPersistentSpell(m_spellProto);
    m_isSingleTarget = IsSingleTargetSpell(spellproto);

    if(GetSpellMaxDuration(m_spellProto) == -1 || m_isPassive && m_spellProto->DurationIndex == 0)
        m_permanent = true;

    m_isRemovedOnShapeLost = (m_caster_guid==m_target->GetGUID() &&
                              m_spellProto->Stances &&
                            !(m_spellProto->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) &&
                            !(m_spellProto->Attributes & SPELL_ATTR_NOT_SHAPESHIFT));

    Player* modOwner = caster && caster->GetObjectGuid().IsUnit() ? ((Unit*)caster)->GetSpellModOwner() : NULL;

    m_procCharges = m_spellProto->procCharges;
    if(modOwner)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);

    if (caster && caster->GetObjectGuid().IsUnit() && m_spellProto->Id == 22959)                // Improved Scorch
    {
        // Glyph of Improved Scorch
        if (Aura* glyph = ((Unit*)caster)->GetDummyAura(56371))
            m_stackAmount = glyph->GetModifier()->m_amount;
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        m_auras[i] = NULL;
}

void SpellAuraHolder::AddAura(Aura *aura, SpellEffectIndex index)
{
    m_auras[index] = aura;
    m_auraFlags |= (1 << index);
}

void SpellAuraHolder::RemoveAura(SpellEffectIndex index)
{
    m_auras[index] = NULL;
    m_auraFlags &= ~(1 << index);
}

void SpellAuraHolder::ApplyAuraModifiers(bool apply, bool real)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX && !IsDeleted(); ++i)
        if (Aura *aur = GetAuraByEffectIndex(SpellEffectIndex(i)))
            aur->ApplyModifier(apply, real);
}

void SpellAuraHolder::_AddSpellAuraHolder()
{
    if (!GetId())
        return;
    if(!m_target)
        return;

    // Try find slot for aura
    uint8 slot = NULL_AURA_SLOT;

    // Lookup free slot
    if (m_target->GetVisibleAurasCount() < MAX_AURAS)
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

    uint8 flags = 0;
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (m_auras[i])
            flags |= (1 << i);
    }
    flags |= ((GetCasterGUID() == GetTarget()->GetGUID()) ? AFLAG_NOT_CASTER : AFLAG_NONE) | ((GetSpellMaxDuration(m_spellProto) > 0) ? AFLAG_DURATION : AFLAG_NONE) | (IsPositive() ? AFLAG_POSITIVE : AFLAG_NEGATIVE);
    SetAuraFlags(flags);

    SetAuraLevel(caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

    if (IsNeedVisibleSlot(caster))
    {
        SetAuraSlot( slot );
        if(slot < MAX_AURAS)                        // slot found send data to client
        {
            SetVisibleAura(false);
            SendAuraUpdate(false);
        }

        //*****************************************************
        // Update target aura state flag on holder apply
        // TODO: Make it easer
        //*****************************************************

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

void SpellAuraHolder::_RemoveSpellAuraHolder()
{
 // Remove all triggered by aura spells vs unlimited duration
    // except same aura replace case
    if(m_removeMode!=AURA_REMOVE_BY_STACK)
        CleanupTriggeredSpells();

    Unit* caster = GetCaster();

    if(caster && IsPersistent())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId());
        if (dynObj)
            dynObj->RemoveAffected(m_target);
    }

    //passive auras do not get put in slots - said who? ;)
    // Note: but totem can be not accessible for aura target in time remove (to far for find in grid)
    //if(m_isPassive && !(caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem()))
    //    return;

    uint8 slot = GetAuraSlot();

    if(slot >= MAX_AURAS)                                   // slot not set
        return;

    if(m_target->GetVisibleAura(slot) == 0)
        return;

    // unregister aura diminishing (and store last time)
    if (getDiminishGroup() != DIMINISHING_NONE )
        m_target->ApplyDiminishingAura(getDiminishGroup(), false);

    SetAuraFlags(AFLAG_NONE);
    SetAuraLevel(0);
    SetVisibleAura(true);

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
            Unit::SpellAuraHolderMap const& holders = m_target->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator i = holders.begin(); i != holders.end(); ++i)
            {
                SpellEntry const *auraSpellInfo = (*i).second->GetSpellProto();
                if(auraSpellInfo->SpellFamilyName  == m_spellProto->SpellFamilyName &&
                    (auraSpellInfo->SpellFamilyFlags & removeFamilyFlag || auraSpellInfo->SpellFamilyFlags2 & removeFamilyFlag2))
                {
                    found = true;
                    break;
                }
            }
            // this was last holder
            if(!found)
                m_target->ModifyAuraState(AuraState(removeState), false);
        }

        // reset cooldown state for spells
        if(caster && caster->GetTypeId() == TYPEID_PLAYER)
        {
            if ( GetSpellProto()->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE )
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
                ((Player*)caster)->SendCooldownEvent(GetSpellProto());
        }
    }
}

void SpellAuraHolder::CleanupTriggeredSpells()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!m_spellProto->EffectApplyAuraName[i])
            continue;

        uint32 tSpellId = m_spellProto->EffectTriggerSpell[i];
        if(!tSpellId)
            continue;

        SpellEntry const* tProto = sSpellStore.LookupEntry(tSpellId);
        if(!tProto)
            continue;

        if(GetSpellDuration(tProto) != -1)
            continue;

        // needed for spell 43680, maybe others
        // TODO: is there a spell flag, which can solve this in a more sophisticated way?
        if(m_spellProto->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
            GetSpellDuration(m_spellProto) == m_spellProto->EffectAmplitude[i])
            continue;

        m_target->RemoveAurasDueToSpell(tSpellId);
    }
}

bool SpellAuraHolder::ModStackAmount(int32 num)
{
    // Can`t mod
    if (!m_spellProto->StackAmount)
        return true;

    // Modify stack but limit it
    int32 stackAmount = m_stackAmount + num;
    if (stackAmount > (int32)m_spellProto->StackAmount)
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

void SpellAuraHolder::SetStackAmount(uint8 stackAmount)
{
    Unit *target = GetTarget();
    Unit *caster = GetCaster();
    if (!target || !caster)
        return;

    bool refresh = stackAmount >= m_stackAmount;
    if (stackAmount != m_stackAmount)
    {
        m_stackAmount = stackAmount;

        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (Aura *aur = m_auras[i])
            {
                int32 bp = aur->GetBasePoints();
                int32 amount = m_stackAmount * caster->CalculateSpellDamage(target, m_spellProto, SpellEffectIndex(i), &bp);
                // Reapply if amount change
                if (amount != aur->GetModifier()->m_amount)
                {
                    aur->ApplyModifier(false, true);
                    aur->GetModifier()->m_amount = amount;
                    aur->ApplyModifier(true, true);
                }
            }
        }
    }

    if (refresh)
        // Stack increased refresh duration
        RefreshHolder();
    else
        // Stack decreased only send update
        SendAuraUpdate(false);
}

Unit* SpellAuraHolder::GetCaster() const
{
    if(m_caster_guid == m_target->GetGUID())
        return m_target;

    //return ObjectAccessor::GetUnit(*m_target,m_caster_guid);
    //must return caster even if it's in another grid/map
    Unit *unit = ObjectAccessor::GetUnitInWorld(*m_target,m_caster_guid);
    return unit && unit->IsInWorld() ? unit : NULL;
}

bool SpellAuraHolder::IsWeaponBuffCoexistableWith(SpellAuraHolder* ref)
{
    // only item casted spells
    if (!GetCastItemGUID())
        return false;

    // Exclude Debuffs
    if (!IsPositive())
        return false;

    // Exclude Non-generic Buffs [ie: Runeforging] and Executioner-Enchant
    if (GetSpellProto()->SpellFamilyName != SPELLFAMILY_GENERIC || GetId() == 42976)
        return false;

    // Exclude Stackable Buffs [ie: Blood Reserve]
    if (GetSpellProto()->StackAmount)
        return false;

    // only self applied player buffs
    if (m_target->GetTypeId() != TYPEID_PLAYER || m_target->GetGUID() != GetCasterGUID())
        return false;

    Item* castItem = ((Player*)m_target)->GetItemByGuid(GetCastItemGUID());
    if (!castItem)
        return false;

    // Limit to Weapon-Slots
    if (!castItem->IsEquipped() ||
        (castItem->GetSlot() != EQUIPMENT_SLOT_MAINHAND && castItem->GetSlot() != EQUIPMENT_SLOT_OFFHAND))
        return false;

    // form different weapons
    return ref->GetCastItemGUID() && ref->GetCastItemGUID() != GetCastItemGUID();
}

bool SpellAuraHolder::IsNeedVisibleSlot(Unit const* caster) const
{
    bool totemAura = caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem();

    if (m_spellProto->procFlags)
        return true;
    else if (HasAuraWithTriggerEffect(m_spellProto))
        return true;
    else if (IsSpellHaveAura(m_spellProto, SPELL_AURA_MOD_IGNORE_SHAPESHIFT))
        return true;
    else if (IsSpellHaveAura(m_spellProto, SPELL_AURA_262))
        return true;

    // passive auras (except totem auras) do not get placed in the slots
    return !m_isPassive || totemAura || HasAreaAuraEffect(m_spellProto);
}

void SpellAuraHolder::SendAuraUpdate(bool remove)
{
    WorldPacket data(SMSG_AURA_UPDATE);
    data << m_target->GetPackGUID();
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
        data.appendPackGUID(GetCasterGUID());
    }

    if(auraFlags & AFLAG_DURATION)
    {
        // take highest - to display icon even if stun fades
        uint32 max_duration = 0;
        uint32 duration = 0;
        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (Aura *aura = m_auras[i])
            {
                if (uint32(aura->GetAuraMaxDuration()) > max_duration)
                {
                    max_duration = aura->GetAuraMaxDuration();
                    duration = aura->GetAuraDuration();
                }
            }
        }

        data << uint32(max_duration);
        data << uint32(duration);
    }

    m_target->SendMessageToSet(&data, true);
}

void SpellAuraHolder::HandleSpellSpecificBoosts(bool apply)
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
                if (!apply && m_removeMode == AURA_REMOVE_BY_DISPEL || m_removeMode == AURA_REMOVE_BY_SHIELD_BREAK)
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
                break;
            }

            switch(GetId())
            {
                case 11129:                                 // Combustion (remove triggered aura stack)
                {
                    if(!apply)
                        spellId1 = 28682;
                    else
                        return;
                    break;
                }
                case 28682:                                 // Combustion (remove main aura)
                {
                    if(!apply)
                        spellId1 = 11129;
                    else
                        return;
                    break;
                }
                case 44401:                                 // Missile Barrage (triggered)
                case 48108:                                 // Hot Streak (triggered)
                case 57761:                                 // Fireball! (Brain Freeze triggered)
                {
                    // consumed aura
                    if (!apply && m_removeMode != AURA_REMOVE_BY_EXPIRE)
                    {
                        Unit* caster = GetCaster();
                        // Item - Mage T10 2P Bonus
                        if (!caster || !caster->HasAura(70752))
                            return;

                        cast_at_remove = true;
                        spellId1 = 70753;                   // Pushing the Limit
                    }
                    else
                        return;
                    break;
                }
                default:
                    return;
            }
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
            // Shadowflame (DoT)
            else if (m_spellProto->SpellFamilyFlags2 & 0x00000002)
            {
                // Glyph of Shadowflame
                Unit* caster;
                if(!apply)
                    spellId1 = 63311;
                else if(((caster = GetCaster())) && caster->HasAura(63310))
                    spellId1 = 63311;
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
                            caster->CastCustomSpell(caster, 64103, &basepoints0, NULL, NULL, true, NULL);
                            return;
                        }
                    }
                }
                else
                    return;
            }
            // Power Word: Shield
            else if (apply && m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001) && m_spellProto->Mechanic == MECHANIC_SHIELD)
            {
                Unit* caster = GetCaster();
               if(!caster)
                    return;

                // Glyph of Power Word: Shield
                if (Aura* glyph = caster->GetAura(55672, EFFECT_INDEX_0))
                {
                    Aura *shield = GetAuraByEffectIndex(EFFECT_INDEX_0);
                    int32 heal = (glyph->GetModifier()->m_amount * shield->GetModifier()->m_amount)/100;
                    caster->CastCustomSpell(m_target, 56160, &heal, NULL, NULL, true, 0, shield);
                }
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
                                chance = (*itr)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1);
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
        case SPELLFAMILY_DRUID:
        {
            // Barkskin
            if (GetId()==22812 && m_target->HasAura(63057)) // Glyph of Barkskin
                spellId1 = 63058;                           // Glyph - Barkskin 01
            else
                return;
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
            // Freezing Trap Effect
            else if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000008))
            {
                if(!apply)
                {
                    Unit *caster = GetCaster();
                    // Glyph of Freezing Trap
                    if (caster && caster->HasAura(56845))
                    {
                        cast_at_remove = true;
                        spellId1 = 61394;
                    }
                    else
                        return;
                }
                else
                    return;
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
                        spellId1 = 57318;                    // Sanctified Wrath (triggered)
                        // prevent aura deletion, specially in multi-boost case
                        SetInUse(true);
                        m_target->CastCustomSpell(m_target, spellId1, &percent, &percent, NULL, true, NULL);
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
            // second part of spell apply
            switch (GetId())
            {
                case 49039: spellId1 = 50397; break;        // Lichborne

                case 48263:                                 // Frost Presence
                case 48265:                                 // Unholy Presence
                case 48266:                                 // Blood Presence
                {
                    // else part one per 3 pair
                    if (GetId()==48263 || GetId()==48265)   // Frost Presence or Unholy Presence
                    {
                        // Improved Blood Presence
                        int32 heal_pct = 0;
                        if (apply)
                        {
                            Unit::AuraList const& bloodAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                            for(Unit::AuraList::const_iterator itr = bloodAuras.begin(); itr != bloodAuras.end(); ++itr)
                            {
                                // skip same icon
                                if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT &&
                                    (*itr)->GetSpellProto()->SpellIconID == 2636)
                                {
                                    heal_pct = (*itr)->GetModifier()->m_amount;
                                    break;
                                }
                            }
                        }

                        if (heal_pct)
                            m_target->CastCustomSpell(m_target, 63611, &heal_pct, NULL, NULL, true, NULL, NULL, GetCasterGUID());
                        else
                            m_target->RemoveAurasDueToSpell(63611);
                    }
                    else
                        spellId1 = 63611;                   // Improved Blood Presence, trigger for heal

                    if (GetId()==48263 || GetId()==48266)   // Frost Presence or Blood Presence
                    {
                        // Improved Unholy Presence
                        int32 power_pct = 0;
                        if (apply)
                        {
                            Unit::AuraList const& unholyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                            for(Unit::AuraList::const_iterator itr = unholyAuras.begin(); itr != unholyAuras.end(); ++itr)
                            {
                                // skip same icon
                                if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT &&
                                    (*itr)->GetSpellProto()->SpellIconID == 2633)
                                {
                                    power_pct = (*itr)->GetModifier()->m_amount;
                                    break;
                                }
                            }
                        }
                        if (power_pct || !apply)
                            spellId2 = 49772;                   // Unholy Presence, speed part, spell1 used for Improvement presence fit to own presence
                    }
                    else
                        spellId1 = 49772;                       // Unholy Presence move speed

                    if (GetId()==48265 || GetId()==48266)       // Unholy Presence or Blood Presence
                    {
                        // Improved Frost Presence
                        int32 stamina_pct = 0;
                        if (apply)
                        {
                            Unit::AuraList const& frostAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                            for(Unit::AuraList::const_iterator itr = frostAuras.begin(); itr != frostAuras.end(); ++itr)
                            {
                                // skip same icon
                                if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT &&
                                    (*itr)->GetSpellProto()->SpellIconID == 2632)
                                {
                                    stamina_pct = (*itr)->GetModifier()->m_amount;
                                    break;
                                }
                            }
                        }

                        if (stamina_pct)
                            m_target->CastCustomSpell(m_target, 61261, &stamina_pct, NULL, NULL, true, NULL, NULL, GetCasterGUID());
                        else
                            m_target->RemoveAurasDueToSpell(61261);
                    }
                    else
                        spellId1 = 61261;                   // Frost Presence, stamina

                    if (GetId()==48265)                     // Unholy Presence
                    {
                        // Improved Unholy Presence, special case for own presence
                        int32 power_pct = 0;
                        if (apply)
                        {
                            Unit::AuraList const& unholyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                            for(Unit::AuraList::const_iterator itr = unholyAuras.begin(); itr != unholyAuras.end(); ++itr)
                            {
                                // skip same icon
                                if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT &&
                                    (*itr)->GetSpellProto()->SpellIconID == 2633)
                                {
                                    power_pct = (*itr)->GetModifier()->m_amount;
                                    break;
                                }
                            }
                        }

                        if (power_pct)
                        {
                            int32 bp = 5;
                            m_target->CastCustomSpell(m_target, 63622, &bp, &bp, &bp, true, NULL, NULL, GetCasterGUID());
                            m_target->CastCustomSpell(m_target, 65095, &bp, NULL, NULL, true, NULL, NULL, GetCasterGUID());
                        }
                        else
                        {
                            m_target->RemoveAurasDueToSpell(63622);
                            m_target->RemoveAurasDueToSpell(65095);
                        }
                    }
                    break;
                }
            }

            // Improved Blood Presence
            if (GetSpellProto()->SpellIconID == 2636 && m_isPassive)
            {
                // if presence active: Frost Presence or Unholy Presence
                if (apply && (m_target->HasAura(48263) || m_target->HasAura(48265)))
                {
                    Aura* aura = GetAuraByEffectIndex(EFFECT_INDEX_0);
                    if (!aura)
                        return;

                    int32 bp = aura->GetModifier()->m_amount;
                    m_target->CastCustomSpell(m_target, 63611, &bp, NULL, NULL, true, NULL, NULL, GetCasterGUID());
                }
                else
                    m_target->RemoveAurasDueToSpell(63611);
                return;
            }

            // Improved Frost Presence
            if (GetSpellProto()->SpellIconID == 2632 && m_isPassive)
            {
                // if presence active: Unholy Presence or Blood Presence
                if (apply && (m_target->HasAura(48265) || m_target->HasAura(48266)))
                {
                    Aura* aura = GetAuraByEffectIndex(EFFECT_INDEX_0);
                    if (!aura)
                        return;

                    int32 bp = aura->GetModifier()->m_amount;
                    m_target->CastCustomSpell(m_target, 61261, &bp, NULL, NULL, true, NULL, NULL, GetCasterGUID());
                }
                else
                    m_target->RemoveAurasDueToSpell(61261);
                return;
            }

            // Improved Unholy Presence
            if (GetSpellProto()->SpellIconID == 2633 && m_isPassive)
            {
                // if presence active: Unholy Presence
                if (apply && m_target->HasAura(48265))
                {
                    int32 bp = 5;
                    m_target->CastCustomSpell(m_target, 63622, &bp, &bp, &bp, true, NULL, NULL, GetCasterGUID());
                    m_target->CastCustomSpell(m_target, 65095, &bp, NULL, NULL, true, NULL, NULL, GetCasterGUID());
                }
                else
                {
                    m_target->RemoveAurasDueToSpell(63622);
                    m_target->RemoveAurasDueToSpell(65095);
                }

                // if presence active: Frost Presence or Blood Presence
                if (!apply || m_target->HasAura(48263) || m_target->HasAura(48266))
                    spellId1 = 49772;
                else
                    return;
                break;
            }
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
            m_target->CastSpell(m_target, spellId1, true, NULL, NULL, GetCasterGUID());
        if (spellId2 && !IsDeleted())
            m_target->CastSpell(m_target, spellId2, true, NULL, NULL, GetCasterGUID());
        if (spellId3 && !IsDeleted())
            m_target->CastSpell(m_target, spellId3, true, NULL, NULL, GetCasterGUID());
        if (spellId4 && !IsDeleted())
            m_target->CastSpell(m_target, spellId4, true, NULL, NULL, GetCasterGUID());
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

SpellAuraHolder::~SpellAuraHolder()
{
    // note: auras in delete list won't be affected since they clear themselves from holder when adding to deletedAuraslist
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            delete aur;
}

void SpellAuraHolder::Update(uint32 diff)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aura = m_auras[i])
            aura->UpdateAura(diff);

    // Channeled aura required check distance from caster
    if(IsChanneledSpell(m_spellProto) && m_caster_guid != m_target->GetGUID())
    {
        Unit* caster = GetCaster();
        if(!caster)
        {
            m_target->RemoveAurasByCasterSpell(GetId(), GetCasterGUID());
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
                caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
                return;
            }
        }
    }
}

void SpellAuraHolder::RefreshHolder()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aura = m_auras[i])
            aura->SetAuraDuration(aura->GetAuraMaxDuration());

    SendAuraUpdate(false);
}

bool SpellAuraHolder::HasMechanic(uint32 mechanic) const
{
    if (mechanic == m_spellProto->Mechanic)
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] == mechanic)
            return true;
    return false;
}

bool SpellAuraHolder::HasMechanicMask(uint32 mechanicMask) const
{
    if (mechanicMask & (1 << (m_spellProto->Mechanic - 1)))
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] && ((1 << (m_spellProto->EffectMechanic[i] -1)) & mechanicMask))
            return true;
    return false;
}

bool SpellAuraHolder::IsPersistent() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (aur->IsPersistent())
                return true;
    return false;
}

bool SpellAuraHolder::IsPositive() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (!aur->IsPositive())
                return false;
    return true;
}

bool SpellAuraHolder::IsEmptyHolder() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            return false;
    return true;
}

void SpellAuraHolder::UnregisterSingleCastHolder()
{
    if (IsSingleTarget())
    {
        if(Unit* caster = GetCaster())
            caster->GetSingleCastSpellTargets().erase(GetSpellProto());

        m_isSingleTarget = false;
    }
}
