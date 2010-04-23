ALTER TABLE db_version CHANGE COLUMN required_8995_01_mangos_spell_proc_event required_8996_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry=61062;
INSERT INTO spell_proc_event VALUES
(61062, 0x00000000,  3, 0x00000000, 0x00000100, 0x00000000, 0x00004000, 0x00010000, 0.000000, 0.000000, 0);
