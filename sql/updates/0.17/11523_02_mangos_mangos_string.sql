ALTER TABLE db_version CHANGE COLUMN required_11523_01_mangos_command required_11523_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1504,1505,1506,1507,1508);

INSERT INTO mangos_string VALUES
(1504,'AI-Information for Npc Entry %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1505,'AIName: %s (%s) ScriptName: %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1506,'Current phase = %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1507,'Combat-Movement is %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1508,'Melee attacking is %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
