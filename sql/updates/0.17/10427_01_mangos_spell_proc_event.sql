ALTER TABLE db_version CHANGE COLUMN required_10423_01_mangos_spell_chain required_10427_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (16954, 16961);
