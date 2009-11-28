ALTER TABLE db_version CHANGE COLUMN required_8882_03_mangos_spell_bonus_data required_8883_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (63534);
INSERT INTO `spell_proc_event` VALUES
(63534, 0x00000000, 6, 0x00000040, 0x00000000, 0x00000000, 0x00004000, 0x00000000, 0.000000, 0.000000, 0);
