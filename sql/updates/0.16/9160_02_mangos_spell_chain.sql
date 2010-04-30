ALTER TABLE db_version CHANGE COLUMN required_9160_01_mangos_spell_proc_event required_9160_02_mangos_spell_chain bit;

/*Quick Recovery*/
DELETE FROM spell_chain WHERE first_spell = 31244;
INSERT INTO spell_chain VALUES
(31244, 0, 31244, 1, 0),
(31245, 31244, 31244, 2, 0);
