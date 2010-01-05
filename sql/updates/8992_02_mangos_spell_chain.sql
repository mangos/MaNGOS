ALTER TABLE db_version CHANGE COLUMN required_8992_01_mangos_spell_proc_event required_8992_02_mangos_spell_chain bit;

/*Master Tactician*/
DELETE FROM spell_chain WHERE first_spell = 34506;
INSERT INTO spell_chain VALUES
(34506, 0, 34506, 1, 0),
(34507, 34506, 34506, 2, 0),
(34508, 34507, 34506, 3, 0),
(34838, 34508, 34506, 4, 0),
(34839, 34838, 34506, 5, 0);

/*Hunting Party*/
DELETE FROM spell_chain WHERE first_spell = 53290;
INSERT INTO spell_chain VALUES
(53290, 0, 53290, 1, 0),
(53291, 53290, 53290, 2, 0),
(53292, 53291, 53290, 3, 0);

/*Bloodsurge*/
DELETE FROM spell_chain WHERE first_spell = 46913;
INSERT INTO spell_chain VALUES
(46913, 0, 46913, 1, 0),
(46914, 46913, 46913, 2, 0),
(46915, 46914, 46913, 3, 0);

/*Entrapment*/
DELETE FROM spell_chain WHERE first_spell = 19184;
INSERT INTO spell_chain VALUES
(19184, 0, 19184, 1, 0),
(19387, 19184, 19184, 2, 0),
(19388, 19387, 19184, 3, 0);

/*Concussive Barrage*/
DELETE FROM spell_chain WHERE first_spell = 35100;
INSERT INTO spell_chain VALUES
(35100, 0, 35100, 1, 0),
(35102, 35100, 35100, 2, 0);

/*Improved Stormstrike*/
DELETE FROM spell_chain WHERE first_spell = 51521;
INSERT INTO spell_chain VALUES
(51521, 0, 51521, 1, 0),
(51522, 51521, 51521, 2, 0);
