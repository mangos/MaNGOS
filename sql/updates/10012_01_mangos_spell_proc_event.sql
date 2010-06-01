ALTER TABLE db_version CHANGE COLUMN required_10011_01_mangos_spell_proc_event required_10012_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 16246;
INSERT INTO `spell_proc_event` VALUES (16246, 0x00000000,  11, 0x981001C3, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);