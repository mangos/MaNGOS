ALTER TABLE db_version CHANGE COLUMN required_12141_01_mangos_command required_12141_02_mangos_mangos_command bit;

DELETE FROM `mangos_string` WHERE `entry` IN (299, 306, 453);
INSERT INTO `mangos_string` VALUE
(299,'The currency id %u of %s was set to %u!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(306,'Amount',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(453,'No currencies found!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
