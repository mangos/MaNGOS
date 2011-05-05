ALTER TABLE db_version CHANGE COLUMN required_10036_01_mangos_spell_chain required_10036_01_mangos_spell_proc_event bit;

DELETE FROM  `spell_proc_event` WHERE `entry` IN (30881,30883,30884,30885,30886);
INSERT INTO `spell_proc_event` VALUES
(30881, 0x00,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  30);
