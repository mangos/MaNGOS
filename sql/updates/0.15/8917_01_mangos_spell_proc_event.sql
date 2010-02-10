ALTER TABLE db_version CHANGE COLUMN required_8912_01_mangos_spell_proc_event required_8917_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (65661);
INSERT INTO `spell_proc_event` VALUES
(65661, 0x00000000, 15, 0x00400011, 0x00020004, 0x00000000, 0x00000010, 0x00000001, 0.000000, 100.000000,0);
