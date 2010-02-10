ALTER TABLE db_version CHANGE COLUMN required_8893_01_mangos_mangos_string required_8899_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (57989);
INSERT INTO `spell_proc_event` VALUES
(57989, 0x00000000, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0.000000, 0.000000, 0);
