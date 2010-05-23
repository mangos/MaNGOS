ALTER TABLE db_version CHANGE COLUMN required_9924_02_mangos_command required_9957_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (210);
INSERT INTO mangos_string VALUES
(210,'Item \'%i\' (with extended cost %i) already in vendor list.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
