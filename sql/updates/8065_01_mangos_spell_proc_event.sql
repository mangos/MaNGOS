ALTER TABLE db_version CHANGE COLUMN required_8064_01_mangos_spell_chain required_8065_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (47535, 47536, 47537, 58435);
