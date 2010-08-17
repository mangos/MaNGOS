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
#include "DBCStructure.h"
#include "DBCStores.h"

int32 SpellEntry::CalculateSimpleValue(SpellEffectIndex eff) const
{
    if(SpellEffectEntry const* effectEntry = GetSpellEffectEntry(Id, eff))
        return effectEntry->EffectBasePoints;
    return 0;
}

uint32 const* SpellEntry::GetEffectSpellClassMask(SpellEffectIndex eff) const
{
    if(SpellEffectEntry const* effectEntry = GetSpellEffectEntry(Id, eff))
        return &effectEntry->EffectSpellClassMaskA[0];
    return NULL;
}

SpellAuraOptionsEntry const* SpellEntry::GetSpellAuraOptions() const
{
    return SpellAuraOptionsId ? sSpellAuraOptionsStore.LookupEntry(SpellAuraOptionsId) : NULL;
}

SpellAuraRestrictionsEntry const* SpellEntry::GetSpellAuraRestrictions() const
{
    return SpellAuraRestrictionsId ? sSpellAuraRestrictionsStore.LookupEntry(SpellAuraRestrictionsId) : NULL;
}

SpellCastingRequirementsEntry const* SpellEntry::GetSpellCastingRequirements() const
{
    return SpellCastingRequirementsId ? sSpellCastingRequirementsStore.LookupEntry(SpellCastingRequirementsId) : NULL;
}

SpellCategoriesEntry const* SpellEntry::GetSpellCategories() const
{
    return SpellCategoriesId ? sSpellCategoriesStore.LookupEntry(SpellCategoriesId) : NULL;
}

SpellClassOptionsEntry const* SpellEntry::GetSpellClassOptions() const
{
    return SpellClassOptionsId ? sSpellClassOptionsStore.LookupEntry(SpellClassOptionsId) : NULL;
}

SpellCooldownsEntry const* SpellEntry::GetSpellCooldowns() const
{
    return SpellCooldownsId ? sSpellCooldownsStore.LookupEntry(SpellCooldownsId) : NULL;
}

SpellEffectEntry const* SpellEntry::GetSpellEffect(SpellEffectIndex eff) const
{
    return GetSpellEffectEntry(Id, eff);
}

SpellEquippedItemsEntry const* SpellEntry::GetSpellEquippedItems() const
{
    return SpellEquippedItemsId ? sSpellEquippedItemsStore.LookupEntry(SpellEquippedItemsId) : NULL;
}

SpellInterruptsEntry const* SpellEntry::GetSpellInterrupts() const
{
    return SpellInterruptsId ? sSpellInterruptsStore.LookupEntry(SpellInterruptsId) : NULL;
}

SpellLevelsEntry const* SpellEntry::GetSpellLevels() const
{
    return SpellLevelsId ? sSpellLevelsStore.LookupEntry(SpellLevelsId) : NULL;
}

SpellPowerEntry const* SpellEntry::GetSpellPower() const
{
    return SpellPowerId ? sSpellPowerStore.LookupEntry(SpellPowerId) : NULL;
}

SpellReagentsEntry const* SpellEntry::GetSpellReagents() const
{
    return SpellReagentsId ? sSpellReagentsStore.LookupEntry(SpellReagentsId) : NULL;
}

SpellScalingEntry const* SpellEntry::GetSpellScaling() const
{
    return SpellScalingId ? sSpellScalingStore.LookupEntry(SpellScalingId) : NULL;
}

SpellShapeshiftEntry const* SpellEntry::GetSpellShapeshift() const
{
    return SpellShapeshiftId ? sSpellShapeshiftStore.LookupEntry(SpellShapeshiftId) : NULL;
}

SpellTargetRestrictionsEntry const* SpellEntry::GetSpellTargetRestrictions() const
{
    return SpellTargetRestrictionsId ? sSpellTargetRestrictionsStore.LookupEntry(SpellTargetRestrictionsId) : NULL;
}

SpellTotemsEntry const* SpellEntry::GetSpellTotems() const
{
    return SpellTotemsId ? sSpellTotemsStore.LookupEntry(SpellTotemsId) : NULL;
}

uint32 SpellEntry::GetManaCost() const
{
    SpellPowerEntry const* power = GetSpellPower();
    return power ? power->manaCost : 0;
}

uint32 SpellEntry::GetPreventionType() const
{
    SpellCategoriesEntry const* cat = GetSpellCategories();
    return cat ? cat->PreventionType : 0;
}

uint32 SpellEntry::GetCategory() const
{
    SpellCategoriesEntry const* cat = GetSpellCategories();
    return cat ? cat->Category : 0;
}

uint32 SpellEntry::GetStartRecoveryTime() const
{
    SpellCooldownsEntry const* cd = GetSpellCooldowns();
    return cd ? cd->StartRecoveryTime : 0;
}

uint32 SpellEntry::GetMechanic() const
{
    SpellCategoriesEntry const* cat = GetSpellCategories();
    return cat ? cat->Mechanic : 0;
}

uint32 SpellEntry::GetRecoveryTime() const
{
    SpellCooldownsEntry const* cd = GetSpellCooldowns();
    return cd ? cd->RecoveryTime : 0;
}

uint32 SpellEntry::GetCategoryRecoveryTime() const
{
    SpellCooldownsEntry const* cd = GetSpellCooldowns();
    return cd ? cd->CategoryRecoveryTime : 0;
}

uint32 SpellEntry::GetStartRecoveryCategory() const
{
    SpellCategoriesEntry const* cat = GetSpellCategories();
    return cat ? cat->StartRecoveryCategory : 0;
}

uint32 SpellEntry::GetSpellLevel() const
{
    SpellLevelsEntry const* levels = GetSpellLevels();
    return levels ? levels->spellLevel : 0;
}

int32 SpellEntry::GetEquippedItemClass() const
{
    SpellEquippedItemsEntry const* items = GetSpellEquippedItems();
    return items ? items->EquippedItemClass : -1;
}

uint32 SpellEntry::GetSpellFamilyName() const
{
    SpellClassOptionsEntry const* classOpt = GetSpellClassOptions();
    return classOpt ? classOpt->SpellFamilyName : 0;
}

uint32 SpellEntry::GetDmgClass() const
{
    SpellCategoriesEntry const* cat = GetSpellCategories();
    return cat ? cat->DmgClass : 0;
}

uint32 SpellEntry::GetDispel() const
{
    SpellCategoriesEntry const* cat = GetSpellCategories();
    return cat ? cat->Dispel : 0;
}

uint32 SpellEntry::GetMaxAffectedTargets() const
{
    SpellTargetRestrictionsEntry const* target = GetSpellTargetRestrictions();
    return target ? target->MaxAffectedTargets : 0;
}

uint32 SpellEntry::GetStackAmount() const
{
    SpellAuraOptionsEntry const* aura = GetSpellAuraOptions();
    return aura ? aura->StackAmount : 0;
}

uint32 SpellEntry::GetManaCostPercentage() const
{
    SpellPowerEntry const* power = GetSpellPower();
    return power ? power->ManaCostPercentage : 0;
}

uint32 SpellEntry::GetProcCharges() const
{
    SpellAuraOptionsEntry const* aura = GetSpellAuraOptions();
    return aura ? aura->procCharges : 0;
}

uint32 SpellEntry::GetProcChance() const
{
    SpellAuraOptionsEntry const* aura = GetSpellAuraOptions();
    return aura ? aura->procChance : 0;
}

uint32 SpellEntry::GetMaxLevel() const
{
    SpellLevelsEntry const* levels = GetSpellLevels();
    return levels ? levels->maxLevel : 0;
}

uint32 SpellEntry::GetTargetAuraState() const
{
    SpellAuraRestrictionsEntry const* aura = GetSpellAuraRestrictions();
    return aura ? aura->TargetAuraState : 0;
}

uint32 SpellEntry::GetManaPerSecond() const
{
    SpellPowerEntry const* power = GetSpellPower();
    return power ? power->manaPerSecond : 0;
}

uint32 SpellEntry::GetRequiresSpellFocus() const
{
    SpellCastingRequirementsEntry const* castReq = GetSpellCastingRequirements();
    return castReq ? castReq->RequiresSpellFocus : 0;
}

uint32 SpellEntry::GetSpellEffectIdByIndex(SpellEffectIndex index) const
{
    SpellEffectEntry const* effect = GetSpellEffect(index);
    return effect ? effect->Effect : SPELL_EFFECT_NONE;
}

uint32 SpellEntry::GetAuraInterruptFlags() const
{
    SpellInterruptsEntry const* interrupt = GetSpellInterrupts();
    return interrupt ? interrupt->AuraInterruptFlags : 0;
}

uint32 SpellEntry::GetEffectImplicitTargetAByIndex(SpellEffectIndex index) const
{
    SpellEffectEntry const* effect = GetSpellEffect(index);
    return effect ? effect->EffectImplicitTargetA : TARGET_NONE;
}

int32 SpellEntry::GetAreaGroupId() const
{
    SpellCastingRequirementsEntry const* castReq = GetSpellCastingRequirements();
    return castReq ? castReq->AreaGroupId : -1;
}

uint32 SpellEntry::GetFacingCasterFlags() const
{
    SpellCastingRequirementsEntry const* castReq = GetSpellCastingRequirements();
    return castReq ? castReq->FacingCasterFlags : -1;
}

uint32 SpellEntry::GetBaseLevel() const
{
    SpellLevelsEntry const* levels = GetSpellLevels();
    return levels ? levels->baseLevel : 0;
}

uint32 SpellEntry::GetInterruptFlags() const
{
    SpellInterruptsEntry const* interrupt = GetSpellInterrupts();
    return interrupt ? interrupt->InterruptFlags : 0;
}

uint32 SpellEntry::GetTargetCreatureType() const
{
    SpellTargetRestrictionsEntry const* target = GetSpellTargetRestrictions();
    return target ? target->TargetCreatureType : 0;
}

int32 SpellEntry::GetEffectMiscValue(SpellEffectIndex index) const
{
    SpellEffectEntry const* effect = GetSpellEffect(index);
    return effect ? effect->EffectMiscValue : 0;
}

uint32 SpellEntry::GetStances() const
{
    SpellShapeshiftEntry const* ss = GetSpellShapeshift();
    return ss ? ss->Stances : 0;
}

uint32 SpellEntry::GetStancesNot() const
{
    SpellShapeshiftEntry const* ss = GetSpellShapeshift();
    return ss ? ss->StancesNot : 0;
}

uint32 SpellEntry::GetProcFlags() const
{
    SpellAuraOptionsEntry const* aura = GetSpellAuraOptions();
    return aura ? aura->procFlags : 0;
}

uint32 SpellEntry::GetChannelInterruptFlags() const
{
    SpellInterruptsEntry const* interrupt = GetSpellInterrupts();
    return interrupt ? interrupt->ChannelInterruptFlags : 0;
}

uint32 SpellEntry::GetManaCostPerLevel() const
{
    SpellPowerEntry const* power = GetSpellPower();
    return power ? power->manaCostPerlevel : 0;
}

uint32 SpellEntry::GetCasterAuraState() const
{
    SpellAuraRestrictionsEntry const* aura = GetSpellAuraRestrictions();
    return aura ? aura->CasterAuraState : 0;
}

uint32 SpellEntry::GetTargets() const
{
    SpellTargetRestrictionsEntry const* target = GetSpellTargetRestrictions();
    return target ? target->Targets : 0;
}

uint32 SpellEntry::GetEffectApplyAuraNameByIndex(SpellEffectIndex index) const
{
    SpellEffectEntry const* effect = GetSpellEffect(index);
    return effect ? effect->EffectApplyAuraName : 0;
}
