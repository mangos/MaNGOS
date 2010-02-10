ALTER TABLE db_version CHANGE COLUMN required_8980_01_mangos_spell_bonus_data required_8981_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 57352;
INSERT INTO `spell_proc_event` VALUES
(57352, 0x00000000,  0, 0x00000001, 0x00000040, 0x00000000, 0x00010154, 0x00000003, 0.000000, 0.000000, 45);
