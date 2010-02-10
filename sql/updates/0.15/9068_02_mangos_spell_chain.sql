ALTER TABLE db_version CHANGE COLUMN required_9068_01_mangos_spell_proc_event required_9068_02_mangos_spell_chain bit;

/*Righteous Vengeance*/
DELETE FROM spell_chain WHERE first_spell = 53380;
INSERT INTO spell_chain VALUES
(53380, 0, 53380, 1, 0),
(53381, 53380, 53380, 2, 0),
(53382, 53381, 53380, 3, 0);

/*The Art of War*/
DELETE FROM spell_chain WHERE first_spell = 53486;
INSERT INTO spell_chain VALUES
(53486, 0, 53486, 1, 0),
(53488, 53486, 53486, 2, 0);
