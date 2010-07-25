ALTER TABLE db_version CHANGE COLUMN required_10012_01_mangos_spell_proc_event required_10015_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (1463, 13163, 32409, 36111, 70664);

INSERT INTO `spell_proc_event` VALUES
(70664, 0x00000000,  7, 0x00000010, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000, 0.000000,  0);

