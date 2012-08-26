ALTER TABLE db_version CHANGE COLUMN required_10350_02_mangos_command required_10353_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (373, 374, 375);

INSERT INTO mangos_string VALUES
(373,'Response:\n%s ',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(374,'Tickets count: %i\n',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(375,'Player %s not have tickets.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
