ALTER TABLE db_version CHANGE COLUMN required_8294_01_mangos_playercreateinfo_action required_8310_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (64928);
INSERT INTO `spell_proc_event` VALUES
(64928, 0x00000000, 11, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0);
