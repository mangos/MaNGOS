ALTER TABLE db_version CHANGE COLUMN required_10621_01_mangos_quest_template required_10628_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (201);

INSERT INTO mangos_string VALUES
(201,'Object GUID is: %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
