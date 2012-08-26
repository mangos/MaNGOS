ALTER TABLE db_version CHANGE COLUMN required_10331_02_mangos_command required_10342_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1161,1162,1163,1164);

INSERT INTO mangos_string VALUES
(1161,'Criteria:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1162,' [counter]',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1163,'Achievement %u doesn\'t exist.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1164,'Achievement criteria %u doesn\'t exist.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
