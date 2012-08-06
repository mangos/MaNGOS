ALTER TABLE db_version CHANGE COLUMN required_11598_01_mangos_spell_proc_event required_11599_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (67151);
INSERT INTO `spell_proc_event` VALUES
(67151, 0x7F, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000000, 0.000000, 0.000000,  0);
