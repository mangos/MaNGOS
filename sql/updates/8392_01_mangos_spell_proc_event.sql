ALTER TABLE db_version CHANGE COLUMN required_8377_01_mangos_spell_area required_8392_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (47569);
INSERT INTO `spell_proc_event` VALUES
(47569, 0x00000000,  6, 0x00004000, 0x00000000, 0x00000000, 0x00004000, 0x00000000, 0.000000, 0.000000, 0);
