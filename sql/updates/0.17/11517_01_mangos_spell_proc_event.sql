ALTER TABLE db_version CHANGE COLUMN required_11516_01_mangos_spell_proc_event required_11517_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE entry = 57870;
INSERT INTO `spell_proc_event` VALUE
(57870, 0x00,  9, 0x00800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00040000, 0x00040000, 0.000000, 0.000000,  0);