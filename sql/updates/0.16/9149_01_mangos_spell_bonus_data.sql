ALTER TABLE db_version CHANGE COLUMN required_9148_01_mangos_spell_bonus_data required_9149_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (31893,31898,32220,32221,53718,53719,53725,53726);
