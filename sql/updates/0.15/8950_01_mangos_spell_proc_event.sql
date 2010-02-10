ALTER TABLE db_version CHANGE COLUMN required_8946_01_mangos_spell_proc_event required_8950_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (55166);
INSERT INTO `spell_proc_event` VALUES
(55166, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000,0);
