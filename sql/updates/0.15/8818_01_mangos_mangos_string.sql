ALTER TABLE db_version CHANGE COLUMN required_8815_01_mangos_mangos_string required_8818_01_mangos_mangos_string bit;

-- this sql might delete some of your translated strings, if you translated them

DELETE FROM mangos_string WHERE entry in (714, 716, 752, 753, 754, 755, 757, 758, 783);

INSERT INTO mangos_string VALUES (752,'Only the Alliance can use that portal',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
INSERT INTO mangos_string VALUES (753,'Only the Horde can use that portal',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);