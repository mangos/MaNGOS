ALTER TABLE db_version CHANGE COLUMN required_11955_02_mangos_command required_11958_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry=817;
INSERT INTO mangos_string VALUES (817,'Warning: You\'ve entered a no-fly zone and are about to be dismounted!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
