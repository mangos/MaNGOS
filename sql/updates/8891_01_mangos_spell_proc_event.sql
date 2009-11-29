ALTER TABLE db_version CHANGE COLUMN required_8889_01_mangos_spell_pet_auras required_8891_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (64976);
INSERT INTO `spell_proc_event` VALUES
(64976, 0x00000000, 4, 0x00000001, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0.000000, 0.000000, 0);
