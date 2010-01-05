ALTER TABLE db_version CHANGE COLUMN required_8932_01_mangos_spell_chain required_8938_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (50880);
INSERT INTO `spell_proc_event` VALUES
(50880, 0x00000010, 15, 0x00000000, 0x00000800, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 0);
