ALTER TABLE db_version CHANGE COLUMN required_9136_06_mangos_spell_proc_event required_9148_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (31935,24275);
