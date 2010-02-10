ALTER TABLE db_version CHANGE COLUMN required_8993_01_mangos_spell_proc_event required_8995_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry=58677;
INSERT INTO spell_proc_event VALUES
(58677, 0x00000000, 15, 0x00002000, 0x00000000, 0x00000000, 0x00004000, 0x00000000, 0.000000, 0.000000, 0);
