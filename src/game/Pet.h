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

#ifndef MANGOSSERVER_PET_H
#define MANGOSSERVER_PET_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Creature.h"
#include "Unit.h"

enum PetType
{
    SUMMON_PET              = 0,
    HUNTER_PET              = 1,
    GUARDIAN_PET            = 2,
    MINI_PET                = 3,
    PROTECTOR_PET           = 4,                            // work as defensive guardian with mini pet suffix in name
    MAX_PET_TYPE            = 5
};

#define MAX_PET_STABLES         4

// stored in character_pet.slot
enum PetSaveMode
{
    PET_SAVE_AS_DELETED        = -1,                        // not saved in fact
    PET_SAVE_AS_CURRENT        =  0,                        // in current slot (with player)
    PET_SAVE_FIRST_STABLE_SLOT =  1,
    PET_SAVE_LAST_STABLE_SLOT  =  MAX_PET_STABLES,          // last in DB stable slot index (including), all higher have same meaning as PET_SAVE_NOT_IN_SLOT
    PET_SAVE_NOT_IN_SLOT       =  100,                      // for avoid conflict with stable size grow will use 100
    PET_SAVE_REAGENTS          =  101                       // PET_SAVE_NOT_IN_SLOT with reagents return
};

// There might be a lot more
enum PetModeFlags
{
    PET_MODE_UNKNOWN_0         = 0x0000001,
    PET_MODE_UNKNOWN_2         = 0x0000100,
    PET_MODE_DISABLE_ACTIONS   = 0x8000000,

    // autoset in client at summon
    PET_MODE_DEFAULT           = PET_MODE_UNKNOWN_0 | PET_MODE_UNKNOWN_2,
};

enum HappinessState
{
    UNHAPPY = 1,
    CONTENT = 2,
    HAPPY   = 3
};

enum PetSpellState
{
    PETSPELL_UNCHANGED = 0,
    PETSPELL_CHANGED   = 1,
    PETSPELL_NEW       = 2,
    PETSPELL_REMOVED   = 3
};

enum PetSpellType
{
    PETSPELL_NORMAL = 0,
    PETSPELL_FAMILY = 1,
};

struct PetSpell
{
    uint8 active;                                           // use instead enum (not good use *uint8* limited enum in case when value in enum not possitive in *int8*)

    PetSpellState state : 8;
    PetSpellType type   : 8;
};

enum ActionFeedback
{
    FEEDBACK_NONE            = 0,
    FEEDBACK_PET_DEAD        = 1,
    FEEDBACK_NOTHING_TO_ATT  = 2,
    FEEDBACK_CANT_ATT_TARGET = 3
};

enum PetTalk
{
    PET_TALK_SPECIAL_SPELL  = 0,
    PET_TALK_ATTACK         = 1
};

enum PetNameInvalidReason
{
    // custom, not send
    PET_NAME_SUCCESS                                        = 0,

    PET_NAME_INVALID                                        = 1,
    PET_NAME_NO_NAME                                        = 2,
    PET_NAME_TOO_SHORT                                      = 3,
    PET_NAME_TOO_LONG                                       = 4,
    PET_NAME_MIXED_LANGUAGES                                = 6,
    PET_NAME_PROFANE                                        = 7,
    PET_NAME_RESERVED                                       = 8,
    PET_NAME_THREE_CONSECUTIVE                              = 11,
    PET_NAME_INVALID_SPACE                                  = 12,
    PET_NAME_CONSECUTIVE_SPACES                             = 13,
    PET_NAME_RUSSIAN_CONSECUTIVE_SILENT_CHARACTERS          = 14,
    PET_NAME_RUSSIAN_SILENT_CHARACTER_AT_BEGINNING_OR_END   = 15,
    PET_NAME_DECLENSION_DOESNT_MATCH_BASE_NAME              = 16
};

enum ScalingTarget
{
    SCALING_TARGET_ALL          = 0,
    SCALING_TARGET_STAT,
    SCALING_TARGET_RESISTANCE,
    SCALING_TARGET_ATTACKPOWER,
    SCALING_TARGET_DAMAGE,
    SCALING_TARGET_SPELLDAMAGE,
    SCALING_TARGET_HIT,
    SCALING_TARGET_SPELLHIT,
    SCALING_TARGET_EXPERTIZE,
    SCALING_TARGET_POWERREGEN,
    SCALING_TARGET_MAX
};

struct ScalingAction
{
    explicit ScalingAction(ScalingTarget _target, uint32 _stat, bool _apply ) :
                                         target(_target), stat(_stat), apply(_apply)
    {}
    ScalingTarget target;
    uint32        stat;
    bool          apply;
};

typedef UNORDERED_MAP<uint32, PetSpell> PetSpellMap;
typedef std::vector<uint32> AutoSpellList;

#define HAPPINESS_LEVEL_SIZE        333000

#define ACTIVE_SPELLS_MAX           4

#define PET_FOLLOW_DIST  1.0f
#define PET_FOLLOW_ANGLE (M_PI_F/2.0f)

class Player;
struct PetScalingData;

class Pet : public Creature
{
    public:
        explicit Pet(PetType type = MAX_PET_TYPE);
        virtual ~Pet();

        void AddToWorld();
        void RemoveFromWorld();

        PetType getPetType() const { return m_petType; }
        void setPetType(PetType type) { m_petType = type; }
        bool isControlled() const { return getPetType()==SUMMON_PET || getPetType()==HUNTER_PET; }
        bool isTemporarySummoned() const { return m_duration > 0; }

        bool IsPermanentPetFor(Player* owner);              // pet have tab in character windows and set UNIT_FIELD_PETNUMBER

        bool CreateBaseAtCreature(Creature* creature, Unit* owner);
        bool LoadPetFromDB( Player* owner, uint32 petentry = 0, uint32 petnumber = 0, bool current = false, float x = 0.0f, float y = 0.0f, float z = 0.0f );
        void SavePetToDB(PetSaveMode mode);
        void Unsummon(PetSaveMode mode, Unit* owner = NULL);
        static void DeleteFromDB(uint32 guidlow);

        void SetDeathState(DeathState s);                   // overwrite virtual Creature::SetDeathState and Unit::SetDeathState
        void Update(uint32 update_diff, uint32 diff);                           // overwrite virtual Creature::Update and Unit::Update

        uint8 GetPetAutoSpellSize() const { return m_autospells.size(); }
        uint32 GetPetAutoSpellOnPos(uint8 pos) const
        {
            if (pos >= m_autospells.size())
                return 0;
            else
                return m_autospells[pos];
        }

        HappinessState GetHappinessState();
        void GivePetXP(uint32 xp);
        void GivePetLevel(uint32 level);
        void SynchronizeLevelWithOwner();
        bool InitStatsForLevel(uint32 level, Unit* owner = NULL);
        bool HaveInDiet(ItemPrototype const* item) const;
        uint32 GetCurrentFoodBenefitLevel(uint32 itemlevel);
        void SetDuration(int32 dur) { m_duration = dur; }

        bool UpdateStats(Stats stat);
        bool UpdateAllStats();
        void UpdateResistances(uint32 school);
        void UpdateArmor();
        void UpdateMaxHealth();
        void UpdateMaxPower(Powers power);
        void UpdateAttackPowerAndDamage(bool ranged = false);
        void UpdateDamagePhysical(WeaponAttackType attType);
        void UpdateSpellPower();
        void UpdateManaRegen();

        bool CanTakeMoreActiveSpells(uint32 SpellIconID);
        void ToggleAutocast(uint32 spellid, bool apply);

        void ApplyModeFlags(PetModeFlags mode, bool apply);
        PetModeFlags GetModeFlags() const { return m_petModeFlags; }

        bool HasSpell(uint32 spell) const;

        void LearnPetPassives();
        void CastPetAuras(bool current);
        void CastPetAura(PetAura const* aura);

        void Regenerate(Powers power, uint32 diff);
        void RegenerateHealth(uint32 diff);
        float OCTRegenHPPerSpirit();
        float OCTRegenMPPerSpirit();
        void CastPetPassiveAuras(bool current);
        void ApplyScalingBonus(ScalingAction* action);
        void ApplyAllScalingBonuses(bool apply);
        void ApplyStatScalingBonus(Stats stat, bool apply);
        void ApplyResistanceScalingBonus(uint32 school, bool apply);
        void ApplyAttackPowerScalingBonus(bool apply);
        void ApplyDamageScalingBonus(bool apply);
        void ApplySpellDamageScalingBonus(bool apply);
        void ApplyHitScalingBonus(bool apply);
        void ApplySpellHitScalingBonus(bool apply);
        void ApplyExpertizeScalingBonus(bool apply);
        void ApplyPowerregenScalingBonus(bool apply);
        bool ReapplyScalingAura(SpellAuraHolder* holder, SpellEntry const *spellproto, SpellEffectIndex index, int32 basePoints);
        PetScalingData* CalculateScalingData( bool recalculate = false );
        void AddScalingAction(ScalingTarget target, uint32 stat, bool apply);
        void ApplyHappinessBonus(bool apply);

        void _LoadSpellCooldowns();
        void _SaveSpellCooldowns();
        void _LoadAuras(uint32 timediff);
        void _SaveAuras();
        void _LoadSpells();
        void _SaveSpells();

        bool addSpell(uint32 spell_id,ActiveStates active = ACT_DECIDE, PetSpellState state = PETSPELL_NEW, PetSpellType type = PETSPELL_NORMAL);
        bool learnSpell(uint32 spell_id);
        void learnSpellHighRank(uint32 spellid);
        void InitLevelupSpellsForLevel();
        bool unlearnSpell(uint32 spell_id, bool learn_prev, bool clear_ab = true);
        bool removeSpell(uint32 spell_id, bool learn_prev, bool clear_ab = true);
        void CleanupActionBar();

        PetSpellMap     m_spells;
        AutoSpellList   m_autospells;

        void InitPetCreateSpells();

        bool resetTalents(bool no_cost = false);
        static void resetTalentsForAllPetsOf(Player* owner, Pet* online_pet = NULL);
        uint32 resetTalentsCost() const;
        void InitTalentForLevel();

        uint8 GetMaxTalentPointsForLevel(uint32 level);
        uint8 GetFreeTalentPoints() { return GetByteValue(UNIT_FIELD_BYTES_1, 1); }
        void SetFreeTalentPoints(uint8 points) { SetByteValue(UNIT_FIELD_BYTES_1, 1, points); }
        void UpdateFreeTalentPoints(bool resetIfNeed = true);

        uint32  m_resetTalentsCost;
        time_t  m_resetTalentsTime;
        uint32  m_usedTalentCount;

        const uint64& GetAuraUpdateMask() const { return m_auraUpdateMask; }
        void SetAuraUpdateMask(uint8 slot) { m_auraUpdateMask |= (uint64(1) << slot); }
        void ResetAuraUpdateMask() { m_auraUpdateMask = 0; }

        float GetPetFollowAngle() const { return m_petFollowAngle; }
        void SetPetFollowAngle(float angle) { m_petFollowAngle = angle; }

        Unit* GetOwner() const;
        bool GetNeedSave() const { return m_needSave; }
        void SetNeedSave(bool needSave) { m_needSave = needSave; }
        uint8 GetPetCounter() { return m_petCounter; }
        void SetPetCounter(uint8 counter) { m_petCounter = counter; }
        bool SetSummonPosition(float x = 0.0f, float y = 0.0f, float z = 0.0f);
        bool Summon();
        void SetCreateSpellID(uint32 SpellID) { m_createSpellID = SpellID; }
        uint32 GetCreateSpellID() { return m_createSpellID; }
        bool Create (uint32 guidlow, Map *map, uint32 phaseMask, uint32 Entry, uint32 pet_number, Unit* owner);
        bool Create (Unit* owner, uint32 Entry);
        bool IsInWorld() const { return ( !m_loading && !m_removed && Object::IsInWorld()); }

        // overwrite Creature function for name localization back to WorldObject version without localization
        const char* GetNameForLocaleIdx(int32 locale_idx) const { return WorldObject::GetNameForLocaleIdx(locale_idx); }

        DeclinedName const* GetDeclinedNames() const { return m_declinedname; }

        bool    m_removed;                                  // prevent overwrite pet state in DB at next Pet::Update if pet already removed(saved)
    protected:
        uint32  m_happinessTimer;
        PetType m_petType;
        int32   m_duration;                                 // time until unsummon (used mostly for summoned guardians and not used for controlled pets)
        uint64  m_auraUpdateMask;
        bool    m_loading;
        bool    m_needSave;                                 // is pet needed to be saved in DB
        float   m_petFollowAngle;                           // follow angle for the pet
        uint8   m_petCounter;
        PetScalingData*  m_PetScalingData;
        PetScalingData*  m_baseBonusData;
        uint32  m_createSpellID;
        std::queue<ScalingAction> m_scalingQueue;
        uint8   m_HappinessState;

        DeclinedName *m_declinedname;

    private:
        PetModeFlags m_petModeFlags;

        void SaveToDB(uint32, uint8)                        // overwrited of Creature::SaveToDB     - don't must be called
        {
            MANGOS_ASSERT(false);
        }
        void DeleteFromDB()                                 // overwrited of Creature::DeleteFromDB - don't must be called
        {
            MANGOS_ASSERT(false);
        }
};


struct ApplyScalingBonusWithHelper
{
    explicit ApplyScalingBonusWithHelper(ScalingTarget _target, uint32 _stat, bool _apply ) :
                                         target(_target), stat(_stat), apply(_apply)
    {}
    void operator()(Unit* unit) const;
    ScalingTarget target;
    uint32 stat;
    bool apply;
};

struct DoPetActionWithHelper
{
    explicit DoPetActionWithHelper( Player* _owner, uint8 _flag, uint32 _spellid, ObjectGuid _petGuid, ObjectGuid _targetGuid) :
             owner(_owner), flag(_flag), spellid(_spellid), petGuid(_petGuid), targetGuid(_targetGuid)
    {}
    void operator()(Unit* unit) const { unit->DoPetAction(owner, flag, spellid, petGuid, targetGuid); }
    Player* owner;
    uint8 flag;
    uint32 spellid;
    ObjectGuid petGuid;
    ObjectGuid targetGuid;
};

struct DoPetCastWithHelper
{
    explicit DoPetCastWithHelper( Player* _owner, uint8 _cast_count, SpellCastTargets* _targets, SpellEntry const* _spellInfo ) :
             owner(_owner), cast_count(_cast_count), targets(_targets), spellInfo(_spellInfo)
    {}
    void operator()(Unit* unit) const { unit->DoPetCastSpell(owner,cast_count,targets,spellInfo ); }
    Player* owner;
    uint8 cast_count;
    SpellCastTargets* targets;
    SpellEntry const* spellInfo;
};

typedef std::map<uint32,std::string> KnownPetNames;

#endif
