ALTER TABLE db_version CHANGE COLUMN required_8863_01_mangos_spell_proc_event required_8873_01_mangos_spell_proc_event bit;

-- (56816) Rune Strike ()
DELETE FROM `spell_proc_event` WHERE `entry` IN (56816);
INSERT INTO `spell_proc_event` VALUES
(56816, 0x00000000, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000030, 0.000000, 0.000000, 0);
