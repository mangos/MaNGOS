ALTER TABLE db_version CHANGE COLUMN required_9899_01_mangos_spell_bonus_data required_9924_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1027,1028);
INSERT INTO mangos_string VALUES
(1027, 'Log filters state:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1028, 'All log filters set to: %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
