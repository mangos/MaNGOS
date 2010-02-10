ALTER TABLE db_version CHANGE COLUMN required_8310_01_mangos_spell_proc_event required_8342_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (53397);
INSERT INTO spell_proc_event VALUES
(53397, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000, 0);
