ALTER TABLE db_version CHANGE COLUMN required_8996_01_mangos_spell_proc_event required_9001_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry=57345;
INSERT INTO spell_proc_event VALUES
(57345, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 45);
