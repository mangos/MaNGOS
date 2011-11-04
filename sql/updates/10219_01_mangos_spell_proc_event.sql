ALTER TABLE db_version CHANGE COLUMN required_10217_05_mangos_spell_proc_event required_10219_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE entry = 36032;
INSERT INTO `spell_proc_event` VALUES (36032, 0x40,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);
