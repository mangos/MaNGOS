ALTER TABLE db_version CHANGE COLUMN required_12195_01_mangos_areatrigger_teleport required_12195_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry=818;
INSERT INTO mangos_string VALUES
(818,'You can\'t enter Black Morass until you rescue Thrall from Durnholde Keep.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
