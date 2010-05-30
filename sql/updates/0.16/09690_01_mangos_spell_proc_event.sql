ALTER TABLE db_version CHANGE COLUMN required_9663_01_mangos_mangos_string required_9690_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 67361;
INSERT INTO `spell_proc_event` VALUES
(67361, 0x00000040,  7, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  6);