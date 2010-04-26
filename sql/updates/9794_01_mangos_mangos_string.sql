ALTER TABLE db_version CHANGE COLUMN required_9768_01_mangos_command required_9794_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1138, 1139, 1140, 1141);
INSERT INTO mangos_string VALUES
(1138, '=================================================================================',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1139, '| GUID       | Name                 | Race            | Class           | Level |',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1140, '| %10u | %20s | %15s | %15s | %5u |',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1141, '%u - |cffffffff|Hplayer:%s|h[%s]|h|r %s %s %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
