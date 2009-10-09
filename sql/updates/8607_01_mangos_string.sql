ALTER TABLE db_version CHANGE COLUMN required_8600_01_mangos_command required_8607_01_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN(1130, 1131);
INSERT INTO mangos_string VALUES
(1130,'event started %u "%s"',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1131,'event stopped %u "%s"',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
