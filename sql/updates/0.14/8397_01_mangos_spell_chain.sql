ALTER TABLE db_version CHANGE COLUMN required_8394_01_mangos_spell_proc_event required_8397_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE first_spell=7386;
