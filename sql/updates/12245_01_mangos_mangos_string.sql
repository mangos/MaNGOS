ALTER TABLE db_version CHANGE COLUMN required_12241_01_mangos_creature_template required_12245_01_mangos_mangos_string bit;

DELETE FROM `mangos_string` WHERE `entry` IN (167, 176);
INSERT INTO `mangos_string` VALUES
(167,'%s has changed your holy power to %i/%i.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(176,'You have changed holy power of %s to %i/%i.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);