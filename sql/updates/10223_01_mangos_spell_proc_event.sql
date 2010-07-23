ALTER TABLE db_version CHANGE COLUMN required_10219_01_mangos_spell_proc_event required_10223_01_mangos_spell_proc_event bit;

delete from `spell_proc_event` where entry = 71761;
insert into `spell_proc_event` values
(71761, 0x00,  3, 0x00000000, 0x00000000, 0x00000000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0.000000, 0.000000,  0);
