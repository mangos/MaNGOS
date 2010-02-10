ALTER TABLE db_version CHANGE COLUMN required_8882_01_mangos_spell_proc_event required_8882_02_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE first_spell = 56636;

/*Taste for Blood*/
INSERT INTO spell_chain VALUES
 (56636, 0, 56636, 1, 0),
 (56637, 56636, 56636, 2, 0),
 (56638, 56637, 56636, 3, 0);
