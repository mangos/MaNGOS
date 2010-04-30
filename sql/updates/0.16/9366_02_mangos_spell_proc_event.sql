ALTER TABLE db_version CHANGE COLUMN required_9366_01_mangos_spell_bonus_data required_9366_02_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 67228;
INSERT INTO `spell_proc_event` VALUES
(67228, 0x00000004, 11, 0x00000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 0);
