ALTER TABLE db_version CHANGE COLUMN required_9220_01_mangos_spell_proc_event required_9220_02_mangos_spell_chain bit;

/*Divine Guardian*/
DELETE FROM spell_chain WHERE first_spell = 53527;
INSERT INTO spell_chain VALUES
(53527, 0, 53527, 1, 0),
(53530, 53527, 53527, 2, 0);
