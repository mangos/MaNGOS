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
#ifndef MANGOS_SPELLAURAS_H
#define MANGOS_SPELLAURAS_H

#include "SpellAuraDefines.h"
#include "DBCEnums.h"
#include "ObjectGuid.h"

struct Modifier
{
    AuraType m_auraname;
    int32 m_amount;
    int32 m_miscvalue;
    uint32 periodictime;
};

class Unit;
struct SpellEntry;
struct ProcTriggerSpell;

// forward decl
class Aura;

// internal helper
struct ReapplyAffectedPassiveAurasHelper;

class MANGOS_DLL_SPEC SpellAuraHolder
{
    public:
        SpellAuraHolder (SpellEntry const* spellproto, Unit *target, WorldObject *caster, Item *castItem);
        Aura* m_auras[MAX_EFFECT_INDEX];

        void AddAura(Aura *aura, SpellEffectIndex index);
        void RemoveAura(SpellEffectIndex index);
        void ApplyAuraModifiers(bool apply, bool real = false);
        void _AddSpellAuraHolder();
        void _RemoveSpellAuraHolder();
        void BuildUpdatePacket(WorldPacket& data) const;
        void SendAuraUpdate(bool remove) const;
        void HandleSpellSpecificBoosts(bool apply);
        void CleanupTriggeredSpells();

        void setDiminishGroup(DiminishingGroup group) { m_AuraDRGroup = group; }
        DiminishingGroup getDiminishGroup() const { return m_AuraDRGroup; }

        uint32 GetStackAmount() const { return m_stackAmount; }
        void SetStackAmount(uint32 stackAmount);
        bool ModStackAmount(int32 num); // return true if last charge dropped

        Aura* GetAuraByEffectIndex(SpellEffectIndex index) const { return m_auras[index]; }

        uint32 GetId() const { return m_spellProto->Id; }
        SpellEntry const* GetSpellProto() const { return m_spellProto; }

        ObjectGuid const& GetCasterGuid() const { return m_casterGuid; }
        void SetCasterGuid(ObjectGuid guid) { m_casterGuid = guid; }
        ObjectGuid const& GetCastItemGuid() const { return m_castItemGuid; }
        Unit* GetCaster() const;
        Unit* GetTarget() const { return m_target; }
        void SetTarget(Unit* target) { m_target = target; }

        bool IsPermanent() const { return m_permanent; }
        void SetPermanent(bool permanent) { m_permanent = permanent; }
        bool IsPassive() const { return m_isPassive; }
        bool IsDeathPersistent() const { return m_isDeathPersist; }
        bool IsPersistent() const;
        bool IsPositive() const;
        bool IsAreaAura() const;                            // if one from auras of holder applied as area aura
        bool IsWeaponBuffCoexistableWith(SpellAuraHolder const* ref) const;
        bool IsNeedVisibleSlot(Unit const* caster) const;
        bool IsRemovedOnShapeLost() const { return m_isRemovedOnShapeLost; }
        bool IsInUse() const { return m_in_use;}
        bool IsDeleted() const { return m_deleted;}
        bool IsEmptyHolder() const;

        void SetDeleted() { m_deleted = true; }

        void SetInUse(bool state)
        {
            if(state)
                ++m_in_use;
            else
            {
                if(m_in_use)
                    --m_in_use;
            }
        }

        void UpdateHolder(uint32 diff) { SetInUse(true); Update(diff); SetInUse(false); }
        void Update(uint32 diff);
        void RefreshHolder();

        bool IsSingleTarget() const { return m_isSingleTarget; }
        void SetIsSingleTarget(bool val) { m_isSingleTarget = val; }
        void UnregisterSingleCastHolder();

        int32 GetAuraMaxDuration() const { return m_maxDuration; }
        void SetAuraMaxDuration(int32 duration);
        int32 GetAuraDuration() const { return m_duration; }
        void SetAuraDuration(int32 duration) { m_duration = duration; }

        uint8 GetAuraSlot() const { return m_auraSlot; }
        void SetAuraSlot(uint8 slot) { m_auraSlot = slot; }
        uint8 GetAuraFlags() const { return m_auraFlags; }
        void SetAuraFlags(uint8 flags) { m_auraFlags = flags; }
        uint8 GetAuraLevel() const { return m_auraLevel; }
        void SetAuraLevel(uint8 level) { m_auraLevel = level; }
        uint32 GetAuraCharges() const { return m_procCharges; }
        void SetAuraCharges(uint32 charges, bool update = true)
        {
            if (m_procCharges == charges)
                return;
            m_procCharges = charges;

            if (update)
                SendAuraUpdate(false);
        }
        bool DropAuraCharge()                               // return true if last charge dropped
        {
            if (m_procCharges == 0)
                return false;

            m_procCharges--;
            SendAuraUpdate(false);
            return m_procCharges == 0;
        }

        time_t GetAuraApplyTime() const { return m_applyTime; }

        void SetVisibleAura(bool remove) { m_target->SetVisibleAura(m_auraSlot, remove ? 0 : GetId()); }
        void SetRemoveMode(AuraRemoveMode mode) { m_removeMode = mode; }
        void SetLoadedState(ObjectGuid const& casterGUID, ObjectGuid const& itemGUID, uint32 stackAmount, uint32 charges, int32 maxduration, int32 duration)
        {
            m_casterGuid   = casterGUID;
            m_castItemGuid = itemGUID;
            m_procCharges  = charges;
            m_stackAmount  = stackAmount;
            SetAuraMaxDuration(maxduration);
            SetAuraDuration(duration);
        }

        bool HasMechanic(uint32 mechanic) const;
        bool HasMechanicMask(uint32 mechanicMask) const;

        ~SpellAuraHolder();
    private:
        Unit* m_target;
        ObjectGuid m_casterGuid;
        ObjectGuid m_castItemGuid;                          // it is NOT safe to keep a pointer to the item because it may get deleted
        time_t m_applyTime;

        SpellEntry const* m_spellProto;

        uint8 m_auraSlot;                                   // Aura slot on unit (for show in client)
        uint8 m_auraFlags;                                  // Aura info flag (for send data to client)
        uint8 m_auraLevel;                                  // Aura level (store caster level for correct show level dep amount)
        uint32 m_procCharges;                               // Aura charges (0 for infinite)
        uint32 m_stackAmount;                               // Aura stack amount
        int32 m_maxDuration;                                // Max aura duration
        int32 m_duration;                                   // Current time
        int32 m_timeCla;                                    // Timer for power per sec calculation

        AuraRemoveMode m_removeMode:8;                      // Store info for know remove aura reason
        DiminishingGroup m_AuraDRGroup:8;                   // Diminishing

        bool m_permanent:1;
        bool m_isPassive:1;
        bool m_isDeathPersist:1;
        bool m_isRemovedOnShapeLost:1;
        bool m_isSingleTarget:1;                            // true if it's a single target spell and registered at caster - can change at spell steal for example
        bool m_deleted:1;

        uint32 m_in_use;                                    // > 0 while in SpellAuraHolder::ApplyModifiers call/SpellAuraHolder::Update/etc
};

typedef void(Aura::*pAuraHandler)(bool Apply, bool Real);
// Real == true at aura add/remove
// Real == false at aura mod unapply/reapply; when adding/removing dependent aura/item/stat mods
//
// Code in aura handler can be guarded by if(Real) check if it should execution only at real add/remove of aura
//
// MAIN RULE: Code MUST NOT be guarded by if(Real) check if it modifies any stats
//      (percent auras, stats mods, etc)
// Second rule: Code must be guarded by if(Real) check if it modifies object state (start/stop attack, send packets to client, etc)
//
// Other case choice: each code line moved under if(Real) check is mangos speedup,
//      each setting object update field code line moved under if(Real) check is significant mangos speedup, and less server->client data sends
//      each packet sending code moved under if(Real) check is _large_ mangos speedup, and lot less server->client data sends

class MANGOS_DLL_SPEC Aura
{
    friend struct ReapplyAffectedPassiveAurasHelper;
    friend Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem);

    public:
        //aura handlers
        void HandleNULL(bool, bool)
        {
            // NOT IMPLEMENTED
        }
        void HandleUnused(bool, bool)
        {
            // NOT USED BY ANY SPELL OR USELESS
        }
        void HandleNoImmediateEffect(bool, bool)
        {
            // aura not have immediate effect at add/remove and handled by ID in other code place
        }
        void HandleBindSight(bool Apply, bool Real);
        void HandleModPossess(bool Apply, bool Real);
        void HandlePeriodicDamage(bool Apply, bool Real);
        void HandleAuraDummy(bool Apply, bool Real);
        void HandleAuraPeriodicDummy(bool apply, bool Real);
        void HandleModConfuse(bool Apply, bool Real);
        void HandleModCharm(bool Apply, bool Real);
        void HandleModFear(bool Apply, bool Real);
        void HandlePeriodicHeal(bool Apply, bool Real);
        void HandleModAttackSpeed(bool Apply, bool Real);
        void HandleModMeleeRangedSpeedPct(bool apply, bool Real);
        void HandleModCombatSpeedPct(bool apply, bool Real);
        void HandleModThreat(bool Apply, bool Real);
        void HandleModTaunt(bool Apply, bool Real);
        void HandleFeignDeath(bool Apply, bool Real);
        void HandleAuraModDisarm(bool Apply, bool Real);
        void HandleAuraModStalked(bool Apply, bool Real);
        void HandleAuraWaterWalk(bool Apply, bool Real);
        void HandleAuraFeatherFall(bool Apply, bool Real);
        void HandleAuraHover(bool Apply, bool Real);
        void HandleAddModifier(bool Apply, bool Real);

        void HandleAuraModStun(bool Apply, bool Real);
        void HandleModDamageDone(bool Apply, bool Real);
        void HandleAuraUntrackable(bool Apply, bool Real);
        void HandleAuraEmpathy(bool Apply, bool Real);
        void HandleModOffhandDamagePercent(bool apply, bool Real);
        void HandleAuraModRangedAttackPower(bool Apply, bool Real);
        void HandleAuraModIncreaseEnergyPercent(bool Apply, bool Real);
        void HandleAuraModIncreaseHealthPercent(bool Apply, bool Real);
        void HandleAuraModRegenInterrupt(bool Apply, bool Real);
        void HandleModMeleeSpeedPct(bool Apply, bool Real);
        void HandlePeriodicTriggerSpell(bool Apply, bool Real);
        void HandlePeriodicTriggerSpellWithValue(bool apply, bool Real);
        void HandlePeriodicEnergize(bool Apply, bool Real);
        void HandleAuraModResistanceExclusive(bool Apply, bool Real);
        void HandleAuraSafeFall(bool Apply, bool Real);
        void HandleAuraModPetTalentsPoints(bool Apply, bool Real);
        void HandleModStealth(bool Apply, bool Real);
        void HandleInvisibility(bool Apply, bool Real);
        void HandleInvisibilityDetect(bool Apply, bool Real);
        void HandleAuraModTotalHealthPercentRegen(bool Apply, bool Real);
        void HandleAuraModTotalManaPercentRegen(bool Apply, bool Real);
        void HandleAuraModResistance(bool Apply, bool Real);
        void HandleAuraModRoot(bool Apply, bool Real);
        void HandleAuraModSilence(bool Apply, bool Real);
        void HandleAuraModStat(bool Apply, bool Real);
        void HandleDetectAmore(bool Apply, bool Real);
        void HandleAuraModIncreaseSpeed(bool Apply, bool Real);
        void HandleAuraModIncreaseMountedSpeed(bool Apply, bool Real);
        void HandleAuraModIncreaseFlightSpeed(bool Apply, bool Real);
        void HandleAuraModDecreaseSpeed(bool Apply, bool Real);
        void HandleAuraModUseNormalSpeed(bool Apply, bool Real);
        void HandleAuraModIncreaseHealth(bool Apply, bool Real);
        void HandleAuraModIncreaseEnergy(bool Apply, bool Real);
        void HandleAuraModShapeshift(bool Apply, bool Real);
        void HandleAuraModEffectImmunity(bool Apply, bool Real);
        void HandleAuraModStateImmunity(bool Apply, bool Real);
        void HandleAuraModSchoolImmunity(bool Apply, bool Real);
        void HandleAuraModDmgImmunity(bool Apply, bool Real);
        void HandleAuraModDispelImmunity(bool Apply, bool Real);
        void HandleAuraProcTriggerSpell(bool Apply, bool Real);
        void HandleAuraTrackCreatures(bool Apply, bool Real);
        void HandleAuraTrackResources(bool Apply, bool Real);
        void HandleAuraModParryPercent(bool Apply, bool Real);
        void HandleAuraModDodgePercent(bool Apply, bool Real);
        void HandleAuraModBlockPercent(bool Apply, bool Real);
        void HandleAuraModCritPercent(bool Apply, bool Real);
        void HandlePeriodicLeech(bool Apply, bool Real);
        void HandleModHitChance(bool Apply, bool Real);
        void HandleModSpellHitChance(bool Apply, bool Real);
        void HandleAuraModScale(bool Apply, bool Real);
        void HandlePeriodicManaLeech(bool Apply, bool Real);
        void HandlePeriodicHealthFunnel(bool apply, bool Real);
        void HandleModCastingSpeed(bool Apply, bool Real);
        void HandleAuraMounted(bool Apply, bool Real);
        void HandleWaterBreathing(bool Apply, bool Real);
        void HandleModBaseResistance(bool Apply, bool Real);
        void HandleModRegen(bool Apply, bool Real);
        void HandleModPowerRegen(bool Apply, bool Real);
        void HandleModPowerRegenPCT(bool Apply, bool Real);
        void HandleChannelDeathItem(bool Apply, bool Real);
        void HandleDamagePercentTaken(bool Apply, bool Real);
        void HandlePeriodicDamagePCT(bool Apply, bool Real);
        void HandleAuraModAttackPower(bool Apply, bool Real);
        void HandleAuraTransform(bool Apply, bool Real);
        void HandleModSpellCritChance(bool Apply, bool Real);
        void HandleAuraModIncreaseSwimSpeed(bool Apply, bool Real);
        void HandleModPowerCostPCT(bool Apply, bool Real);
        void HandleModPowerCost(bool Apply, bool Real);
        void HandleFarSight(bool Apply, bool Real);
        void HandleModPossessPet(bool Apply, bool Real);
        void HandleModMechanicImmunity(bool Apply, bool Real);
        void HandleModMechanicImmunityMask(bool Apply, bool Real);
        void HandleAuraModSkill(bool Apply, bool Real);
        void HandleModDamagePercentDone(bool Apply, bool Real);
        void HandleModPercentStat(bool Apply, bool Real);
        void HandleModResistancePercent(bool Apply, bool Real);
        void HandleAuraModBaseResistancePCT(bool Apply, bool Real);
        void HandleModShieldBlockPCT(bool Apply, bool Real);
        void HandleAuraTrackStealthed(bool Apply, bool Real);
        void HandleModShieldBlock(bool Apply, bool Real);
        void HandleForceReaction(bool Apply, bool Real);
        void HandleAuraModRangedHaste(bool Apply, bool Real);
        void HandleRangedAmmoHaste(bool Apply, bool Real);
        void HandleModHealingDone(bool Apply, bool Real);
        void HandleModTotalPercentStat(bool Apply, bool Real);
        void HandleAuraModTotalThreat(bool Apply, bool Real);
        void HandleModUnattackable(bool Apply, bool Real);
        void HandleAuraModPacify(bool Apply, bool Real);
        void HandleAuraGhost(bool Apply, bool Real);
        void HandleAuraAllowFlight(bool Apply, bool Real);
        void HandleModRating(bool apply, bool Real);
        void HandleModRatingFromStat(bool apply, bool Real);
        void HandleModTargetResistance(bool apply, bool Real);
        void HandleAuraModAttackPowerPercent(bool apply, bool Real);
        void HandleAuraModRangedAttackPowerPercent(bool apply, bool Real);
        void HandleAuraModRangedAttackPowerOfStatPercent(bool apply, bool Real);
        void HandleAuraModAttackPowerOfStatPercent(bool apply, bool Real);
        void HandleAuraModAttackPowerOfArmor(bool apply, bool Real);
        void HandleSpiritOfRedemption(bool apply, bool Real);
        void HandleModManaRegen(bool apply, bool Real);
        void HandleComprehendLanguage(bool apply, bool Real);
        void HandleShieldBlockValue(bool apply, bool Real);
        void HandleModSpellCritChanceShool(bool apply, bool Real);
        void HandleAuraRetainComboPoints(bool apply, bool Real);
        void HandleModSpellDamagePercentFromStat(bool apply, bool Real);
        void HandleModSpellHealingPercentFromStat(bool apply, bool Real);
        void HandleAuraModDispelResist(bool apply, bool Real);
        void HandleAuraControlVehicle(bool apply, bool Real);
        void HandleModSpellDamagePercentFromAttackPower(bool apply, bool Real);
        void HandleModSpellHealingPercentFromAttackPower(bool apply, bool Real);
        void HandleAuraModPacifyAndSilence(bool Apply, bool Real);
        void HandleAuraModIncreaseMaxHealth(bool apply, bool Real);
        void HandleAuraModExpertise(bool apply, bool Real);
        void HandleForceMoveForward(bool apply, bool Real);
        void HandleAuraModResistenceOfStatPercent(bool apply, bool Real);
        void HandleAuraPowerBurn(bool apply, bool Real);
        void HandleSchoolAbsorb(bool apply, bool Real);
        void HandlePreventFleeing(bool apply, bool Real);
        void HandleManaShield(bool apply, bool Real);
        void HandleArenaPreparation(bool apply, bool Real);
        void HandleAuraMirrorImage(bool apply, bool Real);
        void HandleAuraConvertRune(bool apply, bool Real);
        void HandleAuraIncreaseBaseHealthPercent(bool Apply, bool Real);
        void HandleNoReagentUseAura(bool Apply, bool Real);
        void HandlePhase(bool Apply, bool Real);
        void HandleModTargetArmorPct(bool Apply, bool Real);
        void HandleAuraModAllCritChance(bool Apply, bool Real);
        void HandleAuraOpenStable(bool apply, bool Real);
        void HandleAuraAddMechanicAbilities(bool apply, bool Real);
        void HandleAuraStopNaturalManaRegen(bool apply, bool Real);

        virtual ~Aura();

        void SetModifier(AuraType t, int32 a, uint32 pt, int32 miscValue);
        Modifier*       GetModifier()       { return &m_modifier; }
        Modifier const* GetModifier() const { return &m_modifier; }
        int32 GetMiscValue() const { return m_spellAuraHolder->GetSpellProto()->EffectMiscValue[m_effIndex]; }
        int32 GetMiscBValue() const { return m_spellAuraHolder->GetSpellProto()->EffectMiscValueB[m_effIndex]; }

        SpellEntry const* GetSpellProto() const { return GetHolder()->GetSpellProto(); }
        uint32 GetId() const{ return GetHolder()->GetSpellProto()->Id; }
        ObjectGuid const& GetCastItemGuid() const { return GetHolder()->GetCastItemGuid(); }
        ObjectGuid const& GetCasterGuid() const { return GetHolder()->GetCasterGuid(); }
        Unit* GetCaster() const { return GetHolder()->GetCaster(); }
        Unit* GetTarget() const { return GetHolder()->GetTarget(); }

        SpellEffectIndex GetEffIndex() const{ return m_effIndex; }
        int32 GetBasePoints() const { return m_currentBasePoints; }

        int32 GetAuraMaxDuration() const { return GetHolder()->GetAuraMaxDuration(); }
        int32 GetAuraDuration() const { return GetHolder()->GetAuraDuration(); }
        time_t GetAuraApplyTime() const { return m_applyTime; }
        uint32 GetAuraTicks() const { return m_periodicTick; }
        uint32 GetAuraMaxTicks() const
        {
            int32 maxDuration = GetAuraMaxDuration();
            return maxDuration > 0 && m_modifier.periodictime > 0 ? maxDuration / m_modifier.periodictime : 0;
        }
        uint32 GetStackAmount() const { return GetHolder()->GetStackAmount(); }

        void SetLoadedState(int32 damage, uint32 periodicTime)
        {
            m_modifier.m_amount = damage;
            m_modifier.periodictime = periodicTime;

            if(uint32 maxticks = GetAuraMaxTicks())
                m_periodicTick = maxticks - GetAuraDuration() / m_modifier.periodictime;
        }

        bool IsPositive() { return m_positive; }
        bool IsPersistent() const { return m_isPersistent; }
        bool IsAreaAura() const { return m_isAreaAura; }
        bool IsPeriodic() const { return m_isPeriodic; }
        bool IsInUse() const { return m_in_use; }

        void SetInUse(bool state)
        {
            if(state)
                ++m_in_use;
            else
            {
                if(m_in_use)
                    --m_in_use;
            }
        }
        void ApplyModifier(bool apply, bool Real = false);

        void UpdateAura(uint32 diff) { SetInUse(true); Update(diff); SetInUse(false); }

        void SetRemoveMode(AuraRemoveMode mode) { m_removeMode = mode; }

        virtual Unit* GetTriggerTarget() const { return m_spellAuraHolder->GetTarget(); }

        // add/remove SPELL_AURA_MOD_SHAPESHIFT (36) linked auras
        void HandleShapeshiftBoosts(bool apply);

        void TriggerSpell();
        void TriggerSpellWithValue();

        ClassFamilyMask const& GetAuraSpellClassMask() const { return  m_spellAuraHolder->GetSpellProto()->GetEffectSpellClassMask(m_effIndex); }
        bool isAffectedOnSpell(SpellEntry const *spell) const;
        bool CanProcFrom(SpellEntry const *spell, uint32 procFlag, uint32 EventProcEx, uint32 procEx, bool active, bool useClassMask) const;

        //SpellAuraHolder const* GetHolder() const { return m_spellHolder; }
        SpellAuraHolder* GetHolder() { return m_spellAuraHolder; }
        SpellAuraHolder* const GetHolder() const { return m_spellAuraHolder; }

        bool IsLastAuraOnHolder();

        bool HasMechanic(uint32 mechanic) const;
    protected:
        Aura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster = NULL, Item* castItem = NULL);

        // must be called only from Aura::UpdateAura
        virtual void Update(uint32 diff);

        // must be called only from Aura*::Update
        void PeriodicTick();
        void PeriodicDummyTick();

        bool IsCritFromAbilityAura(Unit* caster, uint32& damage);
        void ReapplyAffectedPassiveAuras();

        Modifier m_modifier;

        time_t m_applyTime;

        int32 m_currentBasePoints;                          // cache SpellEntry::CalculateSimpleValue and use for set custom base points
        int32 m_periodicTimer;                              // Timer for periodic auras
        uint32 m_periodicTick;                              // Tick count pass (including current if use in tick code) from aura apply, used for some tick count dependent aura effects

        AuraRemoveMode m_removeMode:8;                      // Store info for know remove aura reason

        SpellEffectIndex m_effIndex :8;                     // Aura effect index in spell

        bool m_positive:1;
        bool m_isPeriodic:1;
        bool m_isAreaAura:1;
        bool m_isPersistent:1;

        uint32 m_in_use;                                    // > 0 while in Aura::ApplyModifier call/Aura::Update/etc

        SpellAuraHolder* const m_spellAuraHolder;
    private:
        void ReapplyAffectedPassiveAuras(Unit* target, bool owner_mode);
};

class MANGOS_DLL_SPEC AreaAura : public Aura
{
    public:
        AreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster = NULL, Item* castItem = NULL);
        ~AreaAura();
    protected:
        void Update(uint32 diff);
    private:
        float m_radius;
        AreaAuraType m_areaAuraType;
};

class MANGOS_DLL_SPEC PersistentAreaAura : public Aura
{
    public:
        PersistentAreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster = NULL, Item* castItem = NULL);
        ~PersistentAreaAura();
    protected:
        void Update(uint32 diff);
};

class MANGOS_DLL_SPEC SingleEnemyTargetAura : public Aura
{
    friend Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem);

    public:
        ~SingleEnemyTargetAura();
        Unit* GetTriggerTarget() const;

    protected:
        SingleEnemyTargetAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster  = NULL, Item* castItem = NULL);
        ObjectGuid m_castersTargetGuid;
};

Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster = NULL, Item* castItem = NULL);
SpellAuraHolder* CreateSpellAuraHolder(SpellEntry const* spellproto, Unit *target, WorldObject *caster, Item *castItem = NULL);
#endif
