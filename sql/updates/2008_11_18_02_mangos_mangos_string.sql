ALTER TABLE db_version CHANGE COLUMN required_2008_11_18_01_mangos_creature_movement required_2008_11_18_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (251);
INSERT INTO mangos_string VALUES
(251,'Text%d (ID: %i): %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
