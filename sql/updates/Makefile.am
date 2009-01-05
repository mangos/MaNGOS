# Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

## Process this file with automake to produce Makefile.in

## Sub-directories to parse

## Change installation location
#  datadir = mangos/sql/updates
pkgdatadir = $(datadir)/mangos/sql/updates

## Files to be installed
#  Install basic SQL files to datadir
pkgdata_DATA = \
	2008_10_18_01_characters_characters.sql \
	2008_10_18_02_mangos_spell_proc_event.sql \
	2008_10_19_01_mangos_spell_affect.sql \
	2008_10_19_02_mangos_spell_proc_event.sql \
	2008_10_21_01_mangos_mangos_string.sql \
	2008_10_21_02_mangos_command.sql \
	2008_10_21_03_mangos_command.sql \
	2008_10_22_01_mangos_quest_template.sql \
	2008_10_22_02_mangos_mangos_string.sql \
	2008_10_23_01_mangos_command.sql \
	2008_10_23_02_mangos_mangos_string.sql \
	2008_10_23_03_mangos_mangos_string.sql \
	2008_10_23_04_mangos_command.sql \
	2008_10_23_05_mangos_command.sql \
	2008_10_26_01_mangos_mangos_string.sql \
	2008_10_27_01_mangos_npc_option.sql \
	2008_10_27_02_mangos_locales_npc_option.sql \
	2008_10_28_01_mangos_mangos_string.sql \
	2008_10_28_02_mangos_mangos_string.sql \
	2008_10_28_03_mangos_command.sql \
	2008_10_28_04_mangos_command.sql \
	2008_10_29_01_mangos_mangos_string.sql \
	2008_10_29_02_mangos_command.sql \
	2008_10_29_03_mangos_db_version.sql \
	2008_10_29_04_mangos_mangos_string.sql \
	2008_10_29_05_mangos_command.sql \
	2008_10_31_01_mangos_creature_template.sql \
	2008_10_31_02_mangos_mangos_string.sql \
	2008_10_31_03_mangos_command.sql \
	2008_11_01_01_mangos_mangos_string.sql \
	2008_11_01_02_mangos_command.sql \
	2008_11_07_01_characters_character_db_version.sql \
	2008_11_07_02_realmd_realmd_db_version.sql \
	2008_11_07_03_characters_guild_bank_tab.sql \
	2008_11_07_04_realmd_account.sql \
	2008_11_09_01_mangos_command.sql \
	2008_11_09_02_mangos_command.sql \
	2008_11_09_03_mangos_mangos_string.sql \
	2008_11_11_01_mangos_db_script_string.sql \
	2008_11_11_02_mangos_scripts.sql \
	2008_11_12_01_character_character_aura.sql \
	2008_11_14_01_mangos_scripts.sql \
	2008_11_16_01_mangos_command.sql \
	2008_11_18_01_mangos_creature_movement.sql \
	2008_11_18_02_mangos_mangos_string.sql \
	2008_11_27_01_mangos_playercreateinfo_item.sql \
	2008_11_29_01_mangos_spell_proc_event.sql \
	2008_11_29_02_mangos_spell_elixir.sql \
	2008_12_03_01_character_guild_member.sql \
	2008_12_15_01_character_arenas.sql \
	2008_12_15_01_mangos_arenas.sql \
	2008_12_22_01_mangos_creature_equip_template.sql \
	2008_12_22_02_characters_character_pet.sql \
	2008_12_22_03_mangos_item_template.sql \
	2008_12_22_04_mangos_item_template.sql \
	2008_12_22_05_characters_account_data.sql \
	2008_12_22_06_characters_character_achievement.sql \
	2008_12_22_07_mangos_quest_template.sql \
	2008_12_22_08_mangos_milling_loot_template.sql \
	2008_12_22_09_mangos_spell_affect.sql \
	2008_12_22_10_mangos_string.sql \
	2008_12_22_11_mangos_player_classlevelstats.sql \
	2008_12_22_12_mangos_player_levelstats.sql \
	2008_12_22_13_mangos_item_template.sql \
	2008_12_22_14_mangos_playercreateinfo.sql \
	2008_12_22_15_mangos_playercreateinfo_action.sql \
	2008_12_22_16_mangos_playercreateinfo_spell.sql \
	2008_12_22_17_mangos_item_template.sql \
	2008_12_22_18_characters_characters.sql \
	2008_12_22_19_characters_item_instance.sql \
	6936_01_mangos_spell_chain.sql \
	6939_01_mangos_quest_template.sql \
	6940_01_mangos_spell_learn_spell.sql \
	6941_01_mangos_spell_learn_spell.sql \
	6944_01_mangos_mangos_string.sql \
	6958_01_mangos_spell_proc_event.sql \
	6960_01_mangos_command.sql \
	6960_02_mangos_string.sql \
	6961_01_mangos_command.sql \
	6970_01_mangos_playercreateinfo.sql \
	6976_01_realmd_realmd_db_version.sql \
	6976_02_characters_character_db_version.sql \
	7002_01_mangos_spell_chain.sql \
	7015_01_mangos_item_template.sql \
	7024_01_mangos_spell_chain.sql \
	7026_01_mangos_battleground_template.sql \
	7031_01_mangos_spell_proc_event.sql \
	7033_01_mangos_spell_proc_event.sql \
	README

## Additional files to include when running 'make dist'
#  SQL update files, to upgrade database schema from older revisions
EXTRA_DIST = \
	2008_10_18_01_characters_characters.sql \
	2008_10_18_02_mangos_spell_proc_event.sql \
	2008_10_19_01_mangos_spell_affect.sql \
	2008_10_19_02_mangos_spell_proc_event.sql \
	2008_10_21_01_mangos_mangos_string.sql \
	2008_10_21_02_mangos_command.sql \
	2008_10_21_03_mangos_command.sql \
	2008_10_22_01_mangos_quest_template.sql \
	2008_10_22_02_mangos_mangos_string.sql \
	2008_10_23_01_mangos_command.sql \
	2008_10_23_02_mangos_mangos_string.sql \
	2008_10_23_03_mangos_mangos_string.sql \
	2008_10_23_04_mangos_command.sql \
	2008_10_23_05_mangos_command.sql \
	2008_10_26_01_mangos_mangos_string.sql \
	2008_10_27_01_mangos_npc_option.sql \
	2008_10_27_02_mangos_locales_npc_option.sql \
	2008_10_28_01_mangos_mangos_string.sql \
	2008_10_28_02_mangos_mangos_string.sql \
	2008_10_28_03_mangos_command.sql \
	2008_10_28_04_mangos_command.sql \
	2008_10_29_01_mangos_mangos_string.sql \
	2008_10_29_02_mangos_command.sql \
	2008_10_29_03_mangos_db_version.sql \
	2008_10_29_04_mangos_mangos_string.sql \
	2008_10_29_05_mangos_command.sql \
	2008_10_31_01_mangos_creature_template.sql \
	2008_10_31_02_mangos_mangos_string.sql \
	2008_10_31_03_mangos_command.sql \
	2008_11_01_01_mangos_mangos_string.sql \
	2008_11_01_02_mangos_command.sql \
	2008_11_07_01_characters_character_db_version.sql \
	2008_11_07_02_realmd_realmd_db_version.sql \
	2008_11_07_03_characters_guild_bank_tab.sql \
	2008_11_07_04_realmd_account.sql \
	2008_11_09_01_mangos_command.sql \
	2008_11_09_02_mangos_command.sql \
	2008_11_09_03_mangos_mangos_string.sql \
	2008_11_11_01_mangos_db_script_string.sql \
	2008_11_11_02_mangos_scripts.sql \
	2008_11_12_01_character_character_aura.sql \
	2008_11_14_01_mangos_scripts.sql \
	2008_11_16_01_mangos_command.sql \
	2008_11_18_01_mangos_creature_movement.sql \
	2008_11_18_02_mangos_mangos_string.sql \
	2008_11_27_01_mangos_playercreateinfo_item.sql \
	2008_11_29_01_mangos_spell_proc_event.sql \
	2008_11_29_02_mangos_spell_elixir.sql \
	2008_12_03_01_character_guild_member.sql \
	2008_12_15_01_character_arenas.sql \
	2008_12_15_01_mangos_arenas.sql \
	2008_12_22_01_mangos_creature_equip_template.sql \
	2008_12_22_02_characters_character_pet.sql \
	2008_12_22_03_mangos_item_template.sql \
	2008_12_22_04_mangos_item_template.sql \
	2008_12_22_05_characters_account_data.sql \
	2008_12_22_06_characters_character_achievement.sql \
	2008_12_22_07_mangos_quest_template.sql \
	2008_12_22_08_mangos_milling_loot_template.sql \
	2008_12_22_09_mangos_spell_affect.sql \
	2008_12_22_10_mangos_string.sql \
	2008_12_22_11_mangos_player_classlevelstats.sql \
	2008_12_22_12_mangos_player_levelstats.sql \
	2008_12_22_13_mangos_item_template.sql \
	2008_12_22_14_mangos_playercreateinfo.sql \
	2008_12_22_15_mangos_playercreateinfo_action.sql \
	2008_12_22_16_mangos_playercreateinfo_spell.sql \
	2008_12_22_17_mangos_item_template.sql \
	2008_12_22_18_characters_characters.sql \
	2008_12_22_19_characters_item_instance.sql \
	6936_01_mangos_spell_chain.sql \
	6939_01_mangos_quest_template.sql \
	6940_01_mangos_spell_learn_spell.sql \
	6941_01_mangos_spell_learn_spell.sql \
	6944_01_mangos_mangos_string.sql \
	6958_01_mangos_spell_proc_event.sql \
	6960_01_mangos_command.sql \
	6960_02_mangos_string.sql \
	6961_01_mangos_command.sql \
	6970_01_mangos_playercreateinfo.sql \
	6976_01_realmd_realmd_db_version.sql \
	6976_02_characters_character_db_version.sql \
	7002_01_mangos_spell_chain.sql \
	7015_01_mangos_item_template.sql \
	7024_01_mangos_spell_chain.sql \
	7026_01_mangos_battleground_template.sql \
	7031_01_mangos_spell_proc_event.sql \
	7033_01_mangos_spell_proc_event.sql \
	README
