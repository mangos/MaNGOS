ALTER TABLE db_version CHANGE COLUMN required_8514_01_mangos_spell_bonus_data required_8521_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 56372;

INSERT INTO `spell_proc_event` VALUES
(56372, 0x00, 3, 0x00000000, 0x00000080, 0x00000000, 0x00004000, 0x00000000, 0.000000, 0.000000, 0);
