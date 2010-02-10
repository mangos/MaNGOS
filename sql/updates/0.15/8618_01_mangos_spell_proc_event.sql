ALTER TABLE db_version CHANGE COLUMN required_8608_02_mangos_battleground_events required_8618_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 56375;

INSERT INTO `spell_proc_event` VALUES
(56375, 0x00, 3, 0x01000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0.000000, 0.000000, 0);
