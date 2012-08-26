ALTER TABLE db_version CHANGE COLUMN required_10219_01_mangos_spell_proc_event required_10223_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE entry = 71761;
INSERT INTO `spell_proc_event` VALUES
(71761, 0x00,  3, 0x00000000, 0x00000000, 0x00000000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0.000000, 0.000000,  0);
