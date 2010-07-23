ALTER TABLE db_version CHANGE COLUMN required_10217_05_mangos_spell_proc_event required_10219_01_mangos_spell_proc_event bit;

delete from `spell_proc_event` where entry = 36032;
insert into `spell_proc_event` values (36032, 0x40,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);