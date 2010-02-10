ALTER TABLE db_version CHANGE COLUMN required_8723_01_mangos_achievement_criteria_requirement required_8726_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry = 62600;
INSERT INTO spell_proc_event VALUES (62600, 0x00000000,  7, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000,  0);