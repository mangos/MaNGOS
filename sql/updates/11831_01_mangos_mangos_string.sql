ALTER TABLE db_version CHANGE COLUMN required_11827_01_mangos_creature_linking_template required_11831_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry = 1193;
INSERT INTO mangos_string VALUES (1193,'Gear Score of Player %s is %u.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
