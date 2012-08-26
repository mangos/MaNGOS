ALTER TABLE db_version CHANGE COLUMN required_10314_02_mangos_command required_10323_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (372);

INSERT INTO mangos_string VALUES
(372,'No achievement!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
