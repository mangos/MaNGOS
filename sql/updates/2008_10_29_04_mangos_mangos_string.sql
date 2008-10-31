ALTER TABLE db_version CHANGE COLUMN required_2008_10_29_03_mangos_db_version required_2008_10_29_04_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (340,341,342);

INSERT INTO mangos_string VALUES
(340,'%s is now following you.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(341,'%s is not following you.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(342,'%s is now not following you.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
