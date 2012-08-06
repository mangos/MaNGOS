ALTER TABLE db_version CHANGE COLUMN required_11599_01_mangos_spell_proc_event required_11602_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (54925);
INSERT INTO `spell_proc_event` VALUES
(54925, 0x00, 10, 0x00000000, 0x00000000, 0x00000000, 0x00000200, 0x00000200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000000, 0.000000, 0.000000,  0);
