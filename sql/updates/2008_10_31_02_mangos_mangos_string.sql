ALTER TABLE db_version CHANGE COLUMN required_2008_10_31_01_mangos_creature_template required_2008_10_31_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (343,344);

INSERT INTO mangos_string VALUES
(343,'Creature (Entry: %u) cannot be tamed.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(344,'You already have pet.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
