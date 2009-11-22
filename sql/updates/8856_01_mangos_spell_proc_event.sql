ALTER TABLE db_version CHANGE COLUMN required_8851_01_mangos_spell_proc_event required_8856_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (58617);
INSERT INTO `spell_proc_event` VALUES
(58617, 0x00000000, 15, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);
