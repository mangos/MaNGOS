ALTER TABLE db_version CHANGE COLUMN required_9025_01_mangos_spell_proc_event required_9034_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry = '67667';
INSERT INTO spell_proc_event VALUES
(67667, 0x00000000, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 45);
