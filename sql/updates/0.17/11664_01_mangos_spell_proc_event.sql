ALTER TABLE db_version CHANGE COLUMN required_11661_01_mangos_spell_proc_event required_11664_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (12298);
INSERT INTO `spell_proc_event` VALUES
(12298, 0x7F, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000070, 0.000000, 0.000000,  0);
