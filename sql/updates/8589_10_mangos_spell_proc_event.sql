ALTER TABLE db_version CHANGE COLUMN required_8589_09_mangos_spell_chain required_8589_10_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (58642,58676,44401);
