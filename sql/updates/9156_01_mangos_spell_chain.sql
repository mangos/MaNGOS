ALTER TABLE db_version CHANGE COLUMN required_9155_01_mangos_spell_proc_event required_9156_01_mangos_spell_chain bit;

/*Aspect of the Dragonhawk*/
DELETE FROM spell_chain WHERE first_spell = 61846;
INSERT INTO spell_chain VALUES
(61846, 0, 61846, 1, 0),
(61847, 61846, 61846, 2, 0);
