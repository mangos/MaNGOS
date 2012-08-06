ALTER TABLE db_version CHANGE COLUMN required_11205_01_mangos_spell_proc_event required_11214_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1202,1203,1204,1205,1206);

INSERT INTO mangos_string VALUES
(1202,'Spell %u %s = %f (*1.88 = %f) DB = %f AP = %f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1203,'direct heal',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1204,'direct damage',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1205,'dot heal',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1206,'dot damage',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
