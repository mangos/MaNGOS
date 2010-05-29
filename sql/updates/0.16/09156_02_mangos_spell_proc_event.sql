ALTER TABLE db_version CHANGE COLUMN required_9156_01_mangos_spell_chain required_9156_02_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (61846, 61847);
INSERT INTO `spell_proc_event` VALUES
(61846, 0x00000000,  9, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);
