ALTER TABLE db_version CHANGE COLUMN required_11597_01_mangos_spell_proc_event required_11598_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE entry IN (64914);
INSERT INTO `spell_proc_event` VALUE
(64914, 0x00,  8, 0x00000000, 0x00000000, 0x00000000, 0x00080000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);

