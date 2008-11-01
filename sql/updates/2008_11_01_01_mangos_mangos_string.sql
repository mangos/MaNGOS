ALTER TABLE db_version CHANGE COLUMN required_2008_10_31_03_mangos_command required_2008_11_01_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1119,1120,1121);

INSERT INTO mangos_string VALUES
(1119,'You must use male or female as gender.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1120,'You change gender of %s to %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1121,'Your gender changed to %s by %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
