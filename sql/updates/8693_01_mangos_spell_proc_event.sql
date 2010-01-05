ALTER TABLE db_version CHANGE COLUMN required_8688_01_mangos_creature_template required_8693_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry = 63320;
INSERT INTO spell_proc_event VALUES (63320, 0x00000000,  5, 0x00040000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);