ALTER TABLE db_version CHANGE COLUMN required_8847_01_mangos_spell_proc_event required_8847_02_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE first_spell = 20335;

INSERT INTO spell_chain VALUES
 (20335, 0, 20335, 1, 0),
 (20336, 20335, 20335, 2, 0),
 (20337, 20336, 20335, 3, 0);
