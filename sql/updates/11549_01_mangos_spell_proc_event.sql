ALTER TABLE db_version CHANGE COLUMN required_11530_01_mangos_spell_proc_event required_11549_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE entry = 58872;
INSERT INTO `spell_proc_event` VALUE
(58872, 0x7F,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000043, 0.000000, 0.000000,  0);
