ALTER TABLE db_version CHANGE COLUMN required_11673_01_mangos_spell_proc_event required_11678_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (64890);
INSERT INTO `spell_proc_event` VALUES
(64890, 0x00, 10, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00010000, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000002, 0.000000, 0.000000,  0);
