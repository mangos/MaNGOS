ALTER TABLE db_version CHANGE COLUMN required_9291_02_mangos_locales_quest required_9296_01_mangos_spell_chain bit;


/*Black Arrow*/
DELETE FROM spell_chain WHERE spell_id IN (3674,63668,63669,63670,63671,63672);

INSERT INTO spell_chain VALUES
(3674,0,3674,1,0),
(63668,3674,3674,2,0),
(63669,63668,3674,3,0),
(63670,63669,3674,4,0),
(63671,63670,3674,5,0),
(63672,63671,3674,6,0);
