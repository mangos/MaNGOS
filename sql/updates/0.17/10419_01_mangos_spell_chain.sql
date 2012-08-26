ALTER TABLE db_version CHANGE COLUMN required_10416_01_mangos_spell_proc_event required_10419_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE first_spell = 50464;

