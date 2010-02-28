ALTER TABLE db_version CHANGE COLUMN required_9466_01_mangos_mangos_string required_9477_01_mangos_spell_proc_event bit;

/*Glyph of Totem of Wrath*/
DELETE FROM `spell_proc_event` WHERE `entry` = 63280;
INSERT INTO `spell_proc_event` VALUES
(63280, 0x00000000, 11, 0x20000000, 0x00000000, 0x00000000, 0x00004000, 0x00000000, 0x000000, 0.000000,  0);
