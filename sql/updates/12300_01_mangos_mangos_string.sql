ALTER TABLE db_version CHANGE COLUMN required_12298_01_mangos_spell_template required_12300_01_mangos_mangos_string bit;

DELETE FROM `mangos_string` WHERE `entry` IN (152, 155, 156, 157, 158, 160, 549);
INSERT INTO `mangos_string` VALUES
(155,'You take %s from %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(156,'%s took %s from you.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(157,'You give %s to %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(158,'%s gave you %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(549,'Played time: %s Level: %u Money: %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

