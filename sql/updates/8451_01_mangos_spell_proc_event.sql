ALTER TABLE db_version CHANGE COLUMN required_8444_01_mangos_mangos_string required_8451_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry = 53646;
INSERT INTO spell_proc_event VALUES
(53646, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0);
