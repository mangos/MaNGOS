ALTER TABLE db_version CHANGE COLUMN required_8908_01_mangos_spell_chain required_8909_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (64127);
INSERT INTO `spell_proc_event` VALUES
(64127, 0x00000000, 6, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 0);
