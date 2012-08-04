ALTER TABLE db_version CHANGE COLUMN required_11766_01_mangos_spell_proc_event required_11768_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (23921, 50253);