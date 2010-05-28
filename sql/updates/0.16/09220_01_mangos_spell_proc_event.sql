ALTER TABLE db_version CHANGE COLUMN required_9198_01_mangos_mangos_string required_9220_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (53527, 53530);
INSERT INTO `spell_proc_event` VALUES
(53527, 0x00000000, 10, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);
