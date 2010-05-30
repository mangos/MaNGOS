ALTER TABLE db_version CHANGE COLUMN required_9156_02_mangos_spell_proc_event required_9160_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (31244,31245);
INSERT INTO `spell_proc_event` VALUES
(31244, 0x00000000,  8, 0x003E0000, 0x00000009, 0x00000000, 0x00000000, 0x00002034, 0.000000, 0.000000,  0);
