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

#ifndef __SPELL_H
#define __SPELL_H

#include "Common.h"
#include "GridDefines.h"
#include "SharedDefines.h"
#include "DBCEnums.h"
#include "ObjectGuid.h"
#include "LootMgr.h"
#include "Unit.h"
#include "Player.h"

class WorldSession;
class WorldPacket;
class DynamicObj;
class Item;
class GameObject;
class Group;
class Aura;

enum SpellCastFlags
{
    CAST_FLAG_NONE              = 0x00000000,
    CAST_FLAG_HIDDEN_COMBATLOG  = 0x00000001,               // hide in combat log?
    CAST_FLAG_UNKNOWN2          = 0x00000002,
    CAST_FLAG_UNKNOWN3          = 0x00000004,
    CAST_FLAG_UNKNOWN4          = 0x00000008,
    CAST_FLAG_UNKNOWN5          = 0x00000010,
    CAST_FLAG_AMMO              = 0x00000020,               // Projectiles visual
    CAST_FLAG_UNKNOWN7          = 0x00000040,               // !0x41 mask used to call CGTradeSkillInfo::DoRecast
    CAST_FLAG_UNKNOWN8          = 0x00000080,
    CAST_FLAG_UNKNOWN9          = 0x00000100,
    CAST_FLAG_UNKNOWN10         = 0x00000200,
    CAST_FLAG_UNKNOWN11         = 0x00000400,
    CAST_FLAG_PREDICTED_POWER   = 0x00000800,               // wotlk, trigger rune cooldown
    CAST_FLAG_UNKNOWN13         = 0x00001000,
    CAST_FLAG_UNKNOWN14         = 0x00002000,
    CAST_FLAG_UNKNOWN15         = 0x00004000,
    CAST_FLAG_UNKNOWN16         = 0x00008000,
    CAST_FLAG_UNKNOWN17         = 0x00010000,
    CAST_FLAG_ADJUST_MISSILE    = 0x00020000,               // wotlk
    CAST_FLAG_UNKNOWN19         = 0x00040000,               // spell cooldown related (may be category cooldown)
    CAST_FLAG_VISUAL_CHAIN      = 0x00080000,               // wotlk
    CAST_FLAG_UNKNOWN21         = 0x00100000,
    CAST_FLAG_PREDICTED_RUNES   = 0x00200000,               // wotlk, rune cooldown list
    CAST_FLAG_IMMUNITY          = 0x04000000,               // spell cast school imminity info
    CAST_FLAG_UNKNOWN24         = 0x08000000,
    CAST_FLAG_UNKNOWN25         = 0x10000000,
    CAST_FLAG_UNKNOWN26         = 0x20000000,
    CAST_FLAG_HEAL_PREDICTION   = 0x40000000,               // heal prediction
    CAST_FLAG_UNKNOWN28         = 0x80000000,
};

enum SpellFlags
{
    SPELL_FLAG_NORMAL       = 0x00,
    SPELL_FLAG_REFLECTED    = 0x01,     // reflected spell
    SPELL_FLAG_REDIRECTED   = 0x02      // redirected spell
};

enum SpellNotifyPushType
{
    PUSH_IN_FRONT,
    PUSH_IN_FRONT_90,
    PUSH_IN_FRONT_30,
    PUSH_IN_FRONT_15,
    PUSH_IN_BACK,
    PUSH_SELF_CENTER,
    PUSH_DEST_CENTER,
    PUSH_TARGET_CENTER
};

bool IsQuestTameSpell(uint32 spellId);

namespace MaNGOS
{
    struct SpellNotifierPlayer;
    struct SpellNotifierCreatureAndPlayer;
}

class SpellCastTargets;

struct SpellCastTargetsReader
{
    explicit SpellCastTargetsReader(SpellCastTargets& _targets, Unit* _caster) : targets(_targets), caster(_caster) {}

    SpellCastTargets& targets;
    Unit* caster;
};

class SpellCastTargets
{
    public:
        SpellCastTargets();
        ~SpellCastTargets();

        void read(ByteBuffer& data, Unit* caster);
        void write(ByteBuffer& data) const;

        SpellCastTargetsReader ReadForCaster(Unit* caster) { return SpellCastTargetsReader(*this, caster); }
        void ReadAdditionalData(WorldPacket& data, uint8& cast_flags);

        SpellCastTargets& operator=(const SpellCastTargets& target)
        {
            m_unitTarget = target.m_unitTarget;
            m_itemTarget = target.m_itemTarget;
            m_GOTarget   = target.m_GOTarget;

            m_unitTargetGUID    = target.m_unitTargetGUID;
            m_GOTargetGUID      = target.m_GOTargetGUID;
            m_CorpseTargetGUID  = target.m_CorpseTargetGUID;
            m_itemTargetGUID    = target.m_itemTargetGUID;
            m_srcTransportGUID  = target.m_srcTransportGUID;
            m_destTransportGUID = target.m_destTransportGUID;

            m_itemTargetEntry  = target.m_itemTargetEntry;

            m_srcX = target.m_srcX;
            m_srcY = target.m_srcY;
            m_srcZ = target.m_srcZ;

            m_destX = target.m_destX;
            m_destY = target.m_destY;
            m_destZ = target.m_destZ;

            m_strTarget = target.m_strTarget;

            m_targetMask = target.m_targetMask;

            m_elevation = target.m_elevation;
            m_speed = target.m_speed;

            return *this;
        }

        void setUnitTarget(Unit* target);
        ObjectGuid getUnitTargetGuid() const { return m_unitTargetGUID; }
        Unit* getUnitTarget() const { return m_unitTarget; }

        void setDestination(float x, float y, float z);
        void setSource(float x, float y, float z);
        void getDestination(float& x, float& y, float& z) const { x = m_destX; y = m_destY; z = m_destZ; }
        void getSource(float& x, float& y, float& z) const { x = m_srcX; y = m_srcY, z = m_srcZ; }

        void setGOTarget(GameObject* target);
        ObjectGuid getGOTargetGuid() const { return m_GOTargetGUID; }
        GameObject* getGOTarget() const { return m_GOTarget; }

        void setCorpseTarget(Corpse* corpse);
        ObjectGuid getCorpseTargetGuid() const { return m_CorpseTargetGUID; }

        void setItemTarget(Item* item);
        ObjectGuid getItemTargetGuid() const { return m_itemTargetGUID; }
        Item* getItemTarget() const { return m_itemTarget; }
        uint32 getItemTargetEntry() const { return m_itemTargetEntry; }

        void setTradeItemTarget(Player* caster);

        void updateTradeSlotItem()
        {
            if (m_itemTarget && (m_targetMask & TARGET_FLAG_TRADE_ITEM))
            {
                m_itemTargetGUID = m_itemTarget->GetObjectGuid();
                m_itemTargetEntry = m_itemTarget->GetEntry();
            }
        }

        bool IsEmpty() const { return !m_GOTargetGUID && !m_unitTargetGUID && !m_itemTarget && !m_CorpseTargetGUID; }

        void Update(Unit* caster);

        float m_srcX, m_srcY, m_srcZ;
        float m_destX, m_destY, m_destZ;
        std::string m_strTarget;

        float GetElevation() const { return m_elevation; }
        float GetSpeed() const { return m_speed; }

        uint32 m_targetMask;

    private:
        // objects (can be used at spell creating and after Update at casting
        Unit* m_unitTarget;
        GameObject* m_GOTarget;
        Item* m_itemTarget;

        // object GUID/etc, can be used always
        ObjectGuid m_unitTargetGUID;
        ObjectGuid m_GOTargetGUID;
        ObjectGuid m_CorpseTargetGUID;
        ObjectGuid m_itemTargetGUID;
        ObjectGuid m_srcTransportGUID;
        ObjectGuid m_destTransportGUID;
        uint32 m_itemTargetEntry;

        float m_elevation;
        float m_speed;
};

inline ByteBuffer& operator<< (ByteBuffer& buf, SpellCastTargets const& targets)
{
    targets.write(buf);
    return buf;
}

inline ByteBuffer& operator>> (ByteBuffer& buf, SpellCastTargetsReader const& targets)
{
    targets.targets.read(buf, targets.caster);
    return buf;
}

enum SpellState
{
    SPELL_STATE_PREPARING = 0,                              // cast time delay period, non channeled spell
    SPELL_STATE_CASTING   = 1,                              // channeled time period spell casting state
    SPELL_STATE_FINISHED  = 2,                              // cast finished to success or fail
    SPELL_STATE_DELAYED   = 3                               // spell casted but need time to hit target(s)
};

enum SpellTargets
{
    SPELL_TARGETS_HOSTILE,
    SPELL_TARGETS_NOT_FRIENDLY,
    SPELL_TARGETS_NOT_HOSTILE,
    SPELL_TARGETS_FRIENDLY,
    SPELL_TARGETS_AOE_DAMAGE,
    SPELL_TARGETS_ALL
};

typedef std::multimap<uint64, uint64> SpellTargetTimeMap;

class Spell
{
        friend struct MaNGOS::SpellNotifierPlayer;
        friend struct MaNGOS::SpellNotifierCreatureAndPlayer;
        friend void Unit::SetCurrentCastedSpell(Spell* pSpell);

    public:
        void EffectEmpty(SpellEffectEntry const* effect);
        void EffectNULL(SpellEffectEntry const* effect);
        void EffectUnused(SpellEffectEntry const* effect);
        void EffectDistract(SpellEffectEntry const* effect);
        void EffectPull(SpellEffectEntry const* effect);
        void EffectSchoolDMG(SpellEffectEntry const* effect);
        void EffectEnvironmentalDMG(SpellEffectEntry const* effect);
        void EffectInstaKill(SpellEffectEntry const* effect);
        void EffectDummy(SpellEffectEntry const* effect);
        void EffectTeleportUnits(SpellEffectEntry const* effect);
        void EffectApplyAura(SpellEffectEntry const* effect);
        void EffectSendEvent(SpellEffectEntry const* effect);
        void EffectPowerBurn(SpellEffectEntry const* effect);
        void EffectPowerDrain(SpellEffectEntry const* effect);
        void EffectHeal(SpellEffectEntry const* effect);
        void EffectBind(SpellEffectEntry const* effect);
        void EffectHealthLeech(SpellEffectEntry const* effect);
        void EffectQuestComplete(SpellEffectEntry const* effect);
        void EffectCreateItem(SpellEffectEntry const* effect);
        void EffectCreateItem2(SpellEffectEntry const* effect);
        void EffectCreateRandomItem(SpellEffectEntry const* effect);
        void EffectPersistentAA(SpellEffectEntry const* effect);
        void EffectEnergize(SpellEffectEntry const* effect);
        void EffectOpenLock(SpellEffectEntry const* effect);
        void EffectSummonChangeItem(SpellEffectEntry const* effect);
        void EffectProficiency(SpellEffectEntry const* effect);
        void EffectApplyAreaAura(SpellEffectEntry const* effect);
        void EffectSummonType(SpellEffectEntry const* effect);
        void EffectLearnSpell(SpellEffectEntry const* effect);
        void EffectDispel(SpellEffectEntry const* effect);
        void EffectDualWield(SpellEffectEntry const* effect);
        void EffectPickPocket(SpellEffectEntry const* effect);
        void EffectAddFarsight(SpellEffectEntry const* effect);
        void EffectHealMechanical(SpellEffectEntry const* effect);
        void EffectJump(SpellEffectEntry const* effect);
        void EffectTeleUnitsFaceCaster(SpellEffectEntry const* effect);
        void EffectLearnSkill(SpellEffectEntry const* effect);
        void EffectAddHonor(SpellEffectEntry const* effect);
        void EffectTradeSkill(SpellEffectEntry const* effect);
        void EffectEnchantItemPerm(SpellEffectEntry const* effect);
        void EffectEnchantItemTmp(SpellEffectEntry const* effect);
        void EffectTameCreature(SpellEffectEntry const* effect);
        void EffectSummonPet(SpellEffectEntry const* effect);
        void EffectLearnPetSpell(SpellEffectEntry const* effect);
        void EffectWeaponDmg(SpellEffectEntry const* effect);
        void EffectClearQuest(SpellEffectEntry const* effect);
        void EffectForceCast(SpellEffectEntry const* effect);
        void EffectTriggerSpell(SpellEffectEntry const* effect);
        void EffectTriggerMissileSpell(SpellEffectEntry const* effect);
        void EffectThreat(SpellEffectEntry const* effect);
        void EffectRestoreItemCharges(SpellEffectEntry const* effect);
        void EffectHealMaxHealth(SpellEffectEntry const* effect);
        void EffectInterruptCast(SpellEffectEntry const* effect);
        void EffectSummonObjectWild(SpellEffectEntry const* effect);
        void EffectScriptEffect(SpellEffectEntry const* effect);
        void EffectSanctuary(SpellEffectEntry const* effect);
        void EffectAddComboPoints(SpellEffectEntry const* effect);
        void EffectDuel(SpellEffectEntry const* effect);
        void EffectStuck(SpellEffectEntry const* effect);
        void EffectSummonPlayer(SpellEffectEntry const* effect);
        void EffectActivateObject(SpellEffectEntry const* effect);
        void EffectApplyGlyph(SpellEffectEntry const* effect);
        void EffectEnchantHeldItem(SpellEffectEntry const* effect);
        void EffectSummonObject(SpellEffectEntry const* effect);
        void EffectResurrect(SpellEffectEntry const* effect);
        void EffectParry(SpellEffectEntry const* effect);
        void EffectBlock(SpellEffectEntry const* effect);
        void EffectLeapForward(SpellEffectEntry const* effect);
        void EffectLeapBack(SpellEffectEntry const* effect);
        void EffectTransmitted(SpellEffectEntry const* effect);
        void EffectDisEnchant(SpellEffectEntry const* effect);
        void EffectInebriate(SpellEffectEntry const* effect);
        void EffectFeedPet(SpellEffectEntry const* effect);
        void EffectDismissPet(SpellEffectEntry const* effect);
        void EffectReputation(SpellEffectEntry const* effect);
        void EffectSelfResurrect(SpellEffectEntry const* effect);
        void EffectSkinning(SpellEffectEntry const* effect);
        void EffectCharge(SpellEffectEntry const* effect);
        void EffectCharge2(SpellEffectEntry const* effect);
        void EffectProspecting(SpellEffectEntry const* effect);
        void EffectRedirectThreat(SpellEffectEntry const* effect);
        void EffectMilling(SpellEffectEntry const* effect);
        void EffectRenamePet(SpellEffectEntry const* effect);
        void EffectSendTaxi(SpellEffectEntry const* effect);
        void EffectKnockBack(SpellEffectEntry const* effect);
        void EffectPlayerPull(SpellEffectEntry const* effect);
        void EffectDispelMechanic(SpellEffectEntry const* effect);
        void EffectSummonDeadPet(SpellEffectEntry const* effect);
        void EffectSummonAllTotems(SpellEffectEntry const* effect);
        void EffectBreakPlayerTargeting (SpellEffectEntry const* effect);
        void EffectDestroyAllTotems(SpellEffectEntry const* effect);
        void EffectDurabilityDamage(SpellEffectEntry const* effect);
        void EffectSkill(SpellEffectEntry const* effect);
        void EffectTaunt(SpellEffectEntry const* effect);
        void EffectDurabilityDamagePCT(SpellEffectEntry const* effect);
        void EffectModifyThreatPercent(SpellEffectEntry const* effect);
        void EffectResurrectNew(SpellEffectEntry const* effect);
        void EffectAddExtraAttacks(SpellEffectEntry const* effect);
        void EffectSpiritHeal(SpellEffectEntry const* effect);
        void EffectSkinPlayerCorpse(SpellEffectEntry const* effect);
        void EffectStealBeneficialBuff(SpellEffectEntry const* effect);
        void EffectUnlearnSpecialization(SpellEffectEntry const* effect);
        void EffectHealPct(SpellEffectEntry const* effect);
        void EffectEnergisePct(SpellEffectEntry const* effect);
        void EffectTriggerSpellWithValue(SpellEffectEntry const* effect);
        void EffectTriggerRitualOfSummoning(SpellEffectEntry const* effect);
        void EffectKillCreditPersonal(SpellEffectEntry const* effect);
        void EffectKillCreditGroup(SpellEffectEntry const* effect);
        void EffectQuestFail(SpellEffectEntry const* effect);
        void EffectQuestOffer(SpellEffectEntry const* effect);
        void EffectActivateRune(SpellEffectEntry const* effect);
        void EffectTeachTaxiNode(SpellEffectEntry const* effect);
        void EffectTitanGrip(SpellEffectEntry const* effect);
        void EffectEnchantItemPrismatic(SpellEffectEntry const* effect);
        void EffectPlaySound(SpellEffectEntry const* effect);
        void EffectPlayMusic(SpellEffectEntry const* effect);
        void EffectSpecCount(SpellEffectEntry const* effect);
        void EffectActivateSpec(SpellEffectEntry const* effect);
        void EffectCancelAura(SpellEffectEntry const* effect);
        void EffectKnockBackFromPosition(SpellEffectEntry const* effect);

        Spell(Unit* caster, SpellEntry const* info, bool triggered, ObjectGuid originalCasterGUID = ObjectGuid(), SpellEntry const* triggeredBy = NULL);
        ~Spell();

        void prepare(SpellCastTargets const* targets, Aura* triggeredByAura = NULL);

        void cancel();

        void update(uint32 difftime);
        void cast(bool skipCheck = false);
        void finish(bool ok = true);
        void TakePower();
        void TakeReagents();
        void TakeCastItem();

        SpellCastResult CheckCast(bool strict);
        SpellCastResult CheckPetCast(Unit* target);

        // handlers
        void handle_immediate();
        uint64 handle_delayed(uint64 t_offset);
        // handler helpers
        void _handle_immediate_phase();
        void _handle_finish_phase();

        SpellCastResult CheckItems();
        SpellCastResult CheckRange(bool strict);
        SpellCastResult CheckPower();
        SpellCastResult CheckOrTakeRunePower(bool take);
        SpellCastResult CheckCasterAuras() const;

        int32 CalculateDamage(SpellEffectIndex i, Unit* target) { return m_caster->CalculateSpellDamage(target, m_spellInfo, i, &m_currentBasePoints[i]); }
        static uint32 CalculatePowerCost(SpellEntry const* spellInfo, Unit* caster, Spell const* spell = NULL, Item* castItem = NULL);

        bool HaveTargetsForEffect(SpellEffectIndex effect) const;
        void Delayed();
        void DelayedChannel();
        uint32 getState() const { return m_spellState; }
        void setState(uint32 state) { m_spellState = state; }

        void DoCreateItem(SpellEffectEntry const* effect, uint32 itemtype);

        void WriteSpellGoTargets(WorldPacket* data);
        void WriteAmmoToPacket(WorldPacket* data);

        typedef std::list<Unit*> UnitList;
        void FillTargetMap();
        void SetTargetMap(SpellEffectIndex effIndex, uint32 targetMode, UnitList& targetUnitMap);

        void FillAreaTargets(UnitList& targetUnitMap, float radius, SpellNotifyPushType pushType, SpellTargets spellTargets, WorldObject* originalCaster = NULL);
        void FillRaidOrPartyTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, bool raid, bool withPets, bool withcaster);
        void FillRaidOrPartyManaPriorityTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, uint32 count, bool raid, bool withPets, bool withcaster);
        void FillRaidOrPartyHealthPriorityTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, uint32 count, bool raid, bool withPets, bool withcaster);

        // Returns a target that was filled by SPELL_SCRIPT_TARGET (or selected victim) Can return NULL
        Unit* GetPrefilledUnitTargetOrUnitTarget(SpellEffectIndex effIndex) const;

        template<typename T> WorldObject* FindCorpseUsing();

        bool CheckTarget(Unit* target, SpellEffectIndex eff);
        bool CanAutoCast(Unit* target);

        static void MANGOS_DLL_SPEC SendCastResult(Player* caster, SpellEntry const* spellInfo, uint8 cast_count, SpellCastResult result);
        void SendCastResult(SpellCastResult result);
        void SendSpellStart();
        void SendSpellGo();
        void SendSpellCooldown();
        void SendLogExecute();
        void SendInterrupted(uint8 result);
        void SendChannelUpdate(uint32 time);
        void SendChannelStart(uint32 duration);
        void SendResurrectRequest(Player* target);
        void SendPlaySpellVisual(uint32 SpellID);

        void HandleEffects(Unit* pUnitTarget, Item* pItemTarget, GameObject* pGOTarget, SpellEffectIndex i, float DamageMultiplier = 1.0);
        void HandleThreatSpells();
        // void HandleAddAura(Unit* Target);

        SpellEntry const* m_spellInfo;
        SpellEntry const* m_triggeredBySpellInfo;
        SpellInterruptsEntry const* m_spellInterrupts;
        int32 m_currentBasePoints[MAX_EFFECT_INDEX];        // cache SpellEntry::CalculateSimpleValue and use for set custom base points
        Item* m_CastItem;
        uint8 m_cast_count;
        uint32 m_glyphIndex;
        SpellCastTargets m_targets;

        int32 GetCastTime() const { return m_casttime; }
        uint32 GetCastedTime() { return m_timer; }
        bool IsAutoRepeat() const { return m_autoRepeat; }
        void SetAutoRepeat(bool rep) { m_autoRepeat = rep; }
        void ReSetTimer() { m_timer = m_casttime > 0 ? m_casttime : 0; }
        bool IsNextMeleeSwingSpell() const
        {
            return m_spellInfo->HasAttribute(SPELL_ATTR_ON_NEXT_SWING_1) || m_spellInfo->HasAttribute(SPELL_ATTR_ON_NEXT_SWING_2);
        }
        bool IsRangedSpell() const
        {
            return  m_spellInfo->HasAttribute(SPELL_ATTR_RANGED);
        }
        bool IsChannelActive() const { return m_caster->GetUInt32Value(UNIT_CHANNEL_SPELL) != 0; }
        bool IsMeleeAttackResetSpell() const { return !m_IsTriggeredSpell && m_spellInterrupts && (m_spellInterrupts->InterruptFlags & SPELL_INTERRUPT_FLAG_AUTOATTACK);  }
        bool IsRangedAttackResetSpell() const { return !m_IsTriggeredSpell && IsRangedSpell() && m_spellInterrupts && (m_spellInterrupts->InterruptFlags & SPELL_INTERRUPT_FLAG_AUTOATTACK); }

        bool IsDeletable() const { return !m_referencedFromCurrentSpell && !m_executedCurrently; }
        void SetReferencedFromCurrent(bool yes) { m_referencedFromCurrentSpell = yes; }
        void SetExecutedCurrently(bool yes) { m_executedCurrently = yes; }
        uint64 GetDelayStart() const { return m_delayStart; }
        void SetDelayStart(uint64 m_time) { m_delayStart = m_time; }
        uint64 GetDelayMoment() const { return m_delayMoment; }

        bool IsNeedSendToClient() const;                    // use for hide spell cast for client in case when cast not have client side affect (animation or log entries)
        bool IsTriggeredSpellWithRedundentData() const;     // use for ignore some spell data for triggered spells like cast time, some triggered spells have redundant copy data from main spell for client use purpose

        CurrentSpellTypes GetCurrentContainer();

        // caster types:
        // formal spell caster, in game source of spell affects cast
        Unit* GetCaster() const { return m_caster; }
        // real source of cast affects, explicit caster, or DoT/HoT applier, or GO owner, or wild GO itself. Can be NULL
        WorldObject* GetAffectiveCasterObject() const;
        // limited version returning NULL in cases wild gameobject caster object, need for Aura (auras currently not support non-Unit caster)
        Unit* GetAffectiveCaster() const { return m_originalCasterGUID ? m_originalCaster : m_caster; }
        // m_originalCasterGUID can store GO guid, and in this case this is visual caster
        WorldObject* GetCastingObject() const;

        uint32 GetPowerCost() const { return m_powerCost; }

        void UpdatePointers();                              // must be used at call Spell code after time delay (non triggered spell cast/update spell call/etc)

        bool CheckTargetCreatureType(Unit* target) const;

        void AddTriggeredSpell(SpellEntry const* spellInfo) { m_TriggerSpells.push_back(spellInfo); }
        void AddPrecastSpell(SpellEntry const* spellInfo) { m_preCastSpells.push_back(spellInfo); }
        void AddTriggeredSpell(uint32 spellId);
        void AddPrecastSpell(uint32 spellId);
        void CastPreCastSpells(Unit* target);
        void CastTriggerSpells();

        void CleanupTargetList();
        void ClearCastItem();

        static void SelectMountByAreaAndSkill(Unit* target, SpellEntry const* parentSpell, uint32 spellId75, uint32 spellId150, uint32 spellId225, uint32 spellId300, uint32 spellIdSpecial);

    protected:
        bool HasGlobalCooldown();
        void TriggerGlobalCooldown();
        void CancelGlobalCooldown();

        void SendLoot(ObjectGuid guid, LootType loottype, LockType lockType);
        bool IgnoreItemRequirements() const;                // some item use spells have unexpected reagent data
        void UpdateOriginalCasterPointer();

        Unit* m_caster;

        ObjectGuid m_originalCasterGUID;                    // real source of cast (aura caster/etc), used for spell targets selection
        // e.g. damage around area spell trigered by victim aura and da,age emeies of aura caster
        Unit* m_originalCaster;                             // cached pointer for m_originalCaster, updated at Spell::UpdatePointers()

        Spell** m_selfContainer;                            // pointer to our spell container (if applicable)

        // Spell data
        SpellSchoolMask m_spellSchoolMask;                  // Spell school (can be overwrite for some spells (wand shoot for example)
        WeaponAttackType m_attackType;                      // For weapon based attack
        uint32 m_powerCost;                                 // Calculated spell cost     initialized only in Spell::prepare
        int32 m_casttime;                                   // Calculated spell cast time initialized only in Spell::prepare
        int32 m_duration;
        bool m_canReflect;                                  // can reflect this spell?
        uint8 m_spellFlags;                                 // for spells whose target was changed in cast i.e. due to reflect
        bool m_autoRepeat;
        uint8 m_runesState;

        uint8 m_delayAtDamageCount;
        bool isDelayableNoMore()
        {
            if (m_delayAtDamageCount >= 2)
                return true;

            ++m_delayAtDamageCount;
            return false;
        }

        // Delayed spells system
        uint64 m_delayStart;                                // time of spell delay start, filled by event handler, zero = just started
        uint64 m_delayMoment;                               // moment of next delay call, used internally
        bool m_immediateHandled;                            // were immediate actions handled? (used by delayed spells only)

        // These vars are used in both delayed spell system and modified immediate spell system
        bool m_referencedFromCurrentSpell;                  // mark as references to prevent deleted and access by dead pointers
        bool m_executedCurrently;                           // mark as executed to prevent deleted and access by dead pointers
        bool m_needSpellLog;                                // need to send spell log?
        uint8 m_applyMultiplierMask;                        // by effect: damage multiplier needed?
        float m_damageMultipliers[3];                       // by effect: damage multiplier

        // Current targets, to be used in SpellEffects (MUST BE USED ONLY IN SPELL EFFECTS)
        Unit* unitTarget;
        Item* itemTarget;
        GameObject* gameObjTarget;
        SpellAuraHolder* m_spellAuraHolder;                 // spell aura holder for current target, created only if spell has aura applying effect
        int32 damage;

        // this is set in Spell Hit, but used in Apply Aura handler
        DiminishingLevels m_diminishLevel;
        DiminishingGroup m_diminishGroup;

        // -------------------------------------------
        GameObject* focusObject;

        // Damage and healing in effects need just calculate
        int32 m_damage;                                     // Damage   in effects count here
        int32 m_healing;                                    // Healing in effects count here
        int32 m_healthLeech;                                // Health leech in effects for all targets count here

        //******************************************
        // Spell trigger system
        //******************************************
        bool   m_canTrigger;                                // Can start trigger (m_IsTriggeredSpell can`t use for this)
        uint8  m_negativeEffectMask;                        // Use for avoid sent negative spell procs for additional positive effects only targets
        uint32 m_procAttacker;                              // Attacker trigger flags
        uint32 m_procVictim;                                // Victim   trigger flags
        void   prepareDataForTriggerSystem();

        //*****************************************
        // Spell target subsystem
        //*****************************************
        // Targets store structures and data
        struct TargetInfo
        {
            ObjectGuid targetGUID;
            uint64 timeDelay;
            uint32 HitInfo;
            uint32 damage;
            SpellMissInfo missCondition: 8;
            SpellMissInfo reflectResult: 8;
            uint8  effectMask: 8;
            bool   processed: 1;
        };
        uint8 m_needAliveTargetMask;                        // Mask req. alive targets

        struct GOTargetInfo
        {
            ObjectGuid targetGUID;
            uint64 timeDelay;
            uint8  effectMask: 8;
            bool   processed: 1;
        };

        struct ItemTargetInfo
        {
            Item*  item;
            uint8 effectMask;
        };

        typedef std::list<TargetInfo>     TargetList;
        typedef std::list<GOTargetInfo>   GOTargetList;
        typedef std::list<ItemTargetInfo> ItemTargetList;

        TargetList     m_UniqueTargetInfo;
        GOTargetList   m_UniqueGOTargetInfo;
        ItemTargetList m_UniqueItemInfo;

        void AddUnitTarget(Unit* target, SpellEffectIndex effIndex);
        void AddUnitTarget(ObjectGuid unitGuid, SpellEffectIndex effIndex);
        void AddGOTarget(GameObject* target, SpellEffectIndex effIndex);
        void AddGOTarget(ObjectGuid goGuid, SpellEffectIndex effIndex);
        void AddItemTarget(Item* target, SpellEffectIndex effIndex);
        void DoAllEffectOnTarget(TargetInfo* target);
        void HandleDelayedSpellLaunch(TargetInfo* target);
        void InitializeDamageMultipliers();
        void ResetEffectDamageAndHeal();
        void DoSpellHitOnUnit(Unit* unit, uint32 effectMask);
        void DoAllEffectOnTarget(GOTargetInfo* target);
        void DoAllEffectOnTarget(ItemTargetInfo* target);
        bool IsAliveUnitPresentInTargetList();
        SpellCastResult CanOpenLock(SpellEffectIndex effIndex, uint32 lockid, SkillType& skillid, int32& reqSkillValue, int32& skillValue);
        // -------------------------------------------

        // List For Triggered Spells
        typedef std::list<SpellEntry const*> SpellInfoList;
        SpellInfoList m_TriggerSpells;                      // casted by caster to same targets settings in m_targets at success finish of current spell
        SpellInfoList m_preCastSpells;                      // casted by caster to each target at spell hit before spell effects apply

        uint32 m_spellState;
        uint32 m_timer;

        float m_castPositionX;
        float m_castPositionY;
        float m_castPositionZ;
        float m_castOrientation;
        bool m_IsTriggeredSpell;

        // if need this can be replaced by Aura copy
        // we can't store original aura link to prevent access to deleted auras
        // and in same time need aura data and after aura deleting.
        SpellEntry const* m_triggeredByAuraSpell;

    private:
        // NPC Summonings
        struct CreaturePosition
        {
            CreaturePosition() :
                x(0.0f), y(0.0f), z(0.0f),
                creature(NULL)
            {}

            float x, y, z;
            Creature* creature;
        };
        typedef std::vector<CreaturePosition> CreatureSummonPositions;

        // return true IFF further processing required
        bool DoSummonPet(SpellEffectEntry const* effect);
        bool DoSummonTotem(SpellEffectEntry const* effect, uint8 slot_dbc = 0);
        bool DoSummonWild(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level);
        bool DoSummonCritter(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level);
        bool DoSummonGuardian(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level);
        bool DoSummonPossessed(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level);
        bool DoSummonVehicle(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level);
};

enum ReplenishType
{
    REPLENISH_UNDEFINED = 0,
    REPLENISH_HEALTH    = 20,
    REPLENISH_MANA      = 21,
    REPLENISH_RAGE      = 22
};

namespace MaNGOS
{
    struct MANGOS_DLL_DECL SpellNotifierPlayer              // Currently unused. When put to use this one requires handling for source-location (smilar to below)
    {
        Spell::UnitList& i_data;
        Spell& i_spell;
        const uint32& i_index;
        float i_radius;
        WorldObject* i_originalCaster;

        SpellNotifierPlayer(Spell& spell, Spell::UnitList& data, const uint32& i, float radius)
            : i_data(data), i_spell(spell), i_index(i), i_radius(radius)
        {
            i_originalCaster = i_spell.GetAffectiveCasterObject();
        }

        void Visit(PlayerMapType& m)
        {
            if (!i_originalCaster)
                return;

            for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                Player* pPlayer = itr->getSource();
                if (!pPlayer->isAlive() || pPlayer->IsTaxiFlying())
                    continue;

                if (i_originalCaster->IsFriendlyTo(pPlayer))
                    continue;

                if (pPlayer->IsWithinDist3d(i_spell.m_targets.m_destX, i_spell.m_targets.m_destY, i_spell.m_targets.m_destZ, i_radius))
                    i_data.push_back(pPlayer);
            }
        }
        template<class SKIP> void Visit(GridRefManager<SKIP>&) {}
    };

    struct MANGOS_DLL_DECL SpellNotifierCreatureAndPlayer
    {
        Spell::UnitList* i_data;
        Spell& i_spell;
        SpellNotifyPushType i_push_type;
        float i_radius;
        SpellTargets i_TargetType;
        WorldObject* i_originalCaster;
        WorldObject* i_castingObject;
        bool i_playerControlled;
        float i_centerX;
        float i_centerY;
        float i_centerZ;

        float GetCenterX() const { return i_centerX; }
        float GetCenterY() const { return i_centerY; }

        SpellNotifierCreatureAndPlayer(Spell& spell, Spell::UnitList& data, float radius, SpellNotifyPushType type,
                                       SpellTargets TargetType = SPELL_TARGETS_NOT_FRIENDLY, WorldObject* originalCaster = NULL)
            : i_data(&data), i_spell(spell), i_push_type(type), i_radius(radius), i_TargetType(TargetType),
              i_originalCaster(originalCaster), i_castingObject(i_spell.GetCastingObject())
        {
            if (!i_originalCaster)
                i_originalCaster = i_spell.GetAffectiveCasterObject();
            i_playerControlled = i_originalCaster  ? i_originalCaster->IsControlledByPlayer() : false;

            switch (i_push_type)
            {
                case PUSH_IN_FRONT:
                case PUSH_IN_FRONT_90:
                case PUSH_IN_FRONT_30:
                case PUSH_IN_FRONT_15:
                case PUSH_IN_BACK:
                case PUSH_SELF_CENTER:
                    if (i_castingObject)
                    {
                        i_centerX = i_castingObject->GetPositionX();
                        i_centerY = i_castingObject->GetPositionY();
                    }
                    break;
                case PUSH_DEST_CENTER:
                    if (i_spell.m_targets.m_targetMask & TARGET_FLAG_SOURCE_LOCATION)
                        i_spell.m_targets.getSource(i_centerX, i_centerY, i_centerZ);
                    else
                        i_spell.m_targets.getDestination(i_centerX, i_centerY, i_centerZ);
                    break;
                case PUSH_TARGET_CENTER:
                    if (Unit* target = i_spell.m_targets.getUnitTarget())
                    {
                        i_centerX = target->GetPositionX();
                        i_centerY = target->GetPositionY();
                    }
                    break;
                default:
                    sLog.outError("SpellNotifierCreatureAndPlayer: unsupported PUSH_* case %u.", i_push_type);
            }
        }

        template<class T> inline void Visit(GridRefManager<T>&  m)
        {
            MANGOS_ASSERT(i_data);

            if (!i_originalCaster || !i_castingObject)
                return;

            for (typename GridRefManager<T>::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                // there are still more spells which can be casted on dead, but
                // they are no AOE and don't have such a nice SPELL_ATTR flag
                if ((i_TargetType != SPELL_TARGETS_ALL && !itr->getSource()->isTargetableForAttack(i_spell.m_spellInfo->HasAttribute(SPELL_ATTR_EX3_CAST_ON_DEAD)))
                        // mostly phase check
                        || !itr->getSource()->IsInMap(i_originalCaster))
                    continue;

                switch (i_TargetType)
                {
                    case SPELL_TARGETS_HOSTILE:
                        if (!i_originalCaster->IsHostileTo(itr->getSource()))
                            continue;
                        break;
                    case SPELL_TARGETS_NOT_FRIENDLY:
                        if (i_originalCaster->IsFriendlyTo(itr->getSource()))
                            continue;
                        break;
                    case SPELL_TARGETS_NOT_HOSTILE:
                        if (i_originalCaster->IsHostileTo(itr->getSource()))
                            continue;
                        break;
                    case SPELL_TARGETS_FRIENDLY:
                        if (!i_originalCaster->IsFriendlyTo(itr->getSource()))
                            continue;
                        break;
                    case SPELL_TARGETS_AOE_DAMAGE:
                    {
                        if (itr->getSource()->GetTypeId() == TYPEID_UNIT && ((Creature*)itr->getSource())->IsTotem())
                            continue;

                        if (i_playerControlled)
                        {
                            if (i_originalCaster->IsFriendlyTo(itr->getSource()))
                                continue;
                        }
                        else
                        {
                            if (!i_originalCaster->IsHostileTo(itr->getSource()))
                                continue;
                        }
                    }
                    break;
                    case SPELL_TARGETS_ALL:
                        break;
                    default: continue;
                }

                // we don't need to check InMap here, it's already done some lines above
                switch (i_push_type)
                {
                    case PUSH_IN_FRONT:
                        if (i_castingObject->isInFront((Unit*)(itr->getSource()), i_radius, 2 * M_PI_F / 3))
                            i_data->push_back(itr->getSource());
                        break;
                    case PUSH_IN_FRONT_90:
                        if (i_castingObject->isInFront((Unit*)(itr->getSource()), i_radius, M_PI_F / 2))
                            i_data->push_back(itr->getSource());
                        break;
                    case PUSH_IN_FRONT_30:
                        if (i_castingObject->isInFront((Unit*)(itr->getSource()), i_radius, M_PI_F / 6))
                            i_data->push_back(itr->getSource());
                        break;
                    case PUSH_IN_FRONT_15:
                        if (i_castingObject->isInFront((Unit*)(itr->getSource()), i_radius, M_PI_F / 12))
                            i_data->push_back(itr->getSource());
                        break;
                    case PUSH_IN_BACK:
                        if (i_castingObject->isInBack((Unit*)(itr->getSource()), i_radius, 2 * M_PI_F / 3))
                            i_data->push_back(itr->getSource());
                        break;
                    case PUSH_SELF_CENTER:
                        if (i_castingObject->IsWithinDist((Unit*)(itr->getSource()), i_radius))
                            i_data->push_back(itr->getSource());
                        break;
                    case PUSH_DEST_CENTER:
                        if (itr->getSource()->IsWithinDist3d(i_centerX, i_centerY, i_centerZ, i_radius))
                            i_data->push_back(itr->getSource());
                        break;
                    case PUSH_TARGET_CENTER:
                        if (i_spell.m_targets.getUnitTarget() && i_spell.m_targets.getUnitTarget()->IsWithinDist((Unit*)(itr->getSource()), i_radius))
                            i_data->push_back(itr->getSource());
                        break;
                }
            }
        }

#ifdef WIN32
        template<> inline void Visit(CorpseMapType&) {}
        template<> inline void Visit(GameObjectMapType&) {}
        template<> inline void Visit(DynamicObjectMapType&) {}
        template<> inline void Visit(CameraMapType&) {}
#endif
    };

#ifndef WIN32
    template<> inline void SpellNotifierCreatureAndPlayer::Visit(CorpseMapType&) {}
    template<> inline void SpellNotifierCreatureAndPlayer::Visit(GameObjectMapType&) {}
    template<> inline void SpellNotifierCreatureAndPlayer::Visit(DynamicObjectMapType&) {}
    template<> inline void SpellNotifierCreatureAndPlayer::Visit(CameraMapType&) {}
#endif
}

typedef void(Spell::*pEffect)(SpellEffectEntry const* spellEffect);

class SpellEvent : public BasicEvent
{
    public:
        SpellEvent(Spell* spell);
        virtual ~SpellEvent();

        virtual bool Execute(uint64 e_time, uint32 p_time) override;
        virtual void Abort(uint64 e_time) override;
        virtual bool IsDeletable() const override;
    protected:
        Spell* m_Spell;
};
#endif
