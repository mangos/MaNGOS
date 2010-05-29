ALTER TABLE db_version CHANGE COLUMN required_9288_01_mangos_spell_bonus_data required_9289_01_mangos_spell_proc_event bit;
DELETE FROM `spell_proc_event` WHERE `entry` IN (57870);
INSERT INTO `spell_proc_event` VALUES
(57870, 0x00000000, 9, 0x00800000, 0x00000000, 0x00000000, 0x00040000, 0x00000000, 0.000000, 0.000000, 0);
