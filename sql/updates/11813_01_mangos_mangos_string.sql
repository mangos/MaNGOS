ALTER TABLE db_version CHANGE COLUMN required_11807_01_mangos_gameobject_addon required_11813_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (707);

INSERT INTO mangos_string VALUES (707,'%s does not wish to be disturbed: %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
