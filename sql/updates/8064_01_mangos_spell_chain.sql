ALTER TABLE db_version CHANGE COLUMN required_8060_01_mangos_spell_pet_auras required_8064_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE first_spell = 47535;
INSERT INTO spell_chain VALUES
/*------------------
--(613)Discipline
------------------*/
/*Rapture*/
(47535,0,47535,1,0),
(47536,47535,47535,2,0),
(47537,47536,47535,3,0);
