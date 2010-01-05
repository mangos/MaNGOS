ALTER TABLE db_version CHANGE COLUMN required_8909_01_mangos_spell_proc_event required_8912_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (67353);
INSERT INTO `spell_proc_event` VALUES
(67353, 0x00000000, 7, 0x00008000, 0x00100500, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 0);
