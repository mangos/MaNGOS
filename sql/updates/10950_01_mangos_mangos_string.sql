ALTER TABLE db_version CHANGE COLUMN required_10949_01_mangos_mangos_string required_10950_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1169);

INSERT INTO mangos_string VALUES
(1169,'Scripting library build for different mangosd revision.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
