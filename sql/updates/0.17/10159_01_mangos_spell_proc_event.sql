ALTER TABLE db_version CHANGE COLUMN required_10156_03_mangos_spell_proc_event required_10159_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 48516;
INSERT INTO `spell_proc_event` VALUES
(48516, 0x00,  7, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000, 30);
