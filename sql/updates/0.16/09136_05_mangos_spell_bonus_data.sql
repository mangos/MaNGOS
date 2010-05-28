ALTER TABLE db_version CHANGE COLUMN required_9136_04_mangos_spell_chain required_9136_05_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 8443;
