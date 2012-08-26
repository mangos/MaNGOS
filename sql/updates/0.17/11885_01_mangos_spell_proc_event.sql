ALTER TABLE db_version CHANGE COLUMN required_11876_01_mangos_creature_linking_template required_11885_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry=72413;
INSERT INTO spell_proc_event VALUES (72413, 0x00,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 20.000000, 60);
