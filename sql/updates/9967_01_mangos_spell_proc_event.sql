ALTER TABLE db_version CHANGE COLUMN required_9957_02_mangos_npc_vendor required_9967_01_mangos_spell_proc_event bit;

delete from `spell_proc_event` where entry = 16164;
insert into `spell_proc_event` values (16164, 0x0000001C, 11, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000002, 0.000000, 0.000000,  0);