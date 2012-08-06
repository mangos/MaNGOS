ALTER TABLE db_version CHANGE COLUMN required_11664_01_mangos_spell_proc_event required_11673_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (63086);
INSERT INTO `spell_proc_event` VALUES
(63086, 0x00, 9, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00010000, 0x00010000, 0x00000000, 0x0000000, 0.000000, 0.000000,  0);
