ALTER TABLE db_version CHANGE COLUMN required_9385_01_mangos_command required_9450_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 70664;
INSERT INTO `spell_proc_event` VALUES
(70664, 0x00000000,  7, 0x00000010, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000, 2.000000,  0);
