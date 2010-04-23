ALTER TABLE db_version CHANGE COLUMN required_8841_02_mangos_spell_chain required_8847_01_mangos_spell_proc_event bit;

-- (20335) Heart of the Crusader (Rank 1)
DELETE FROM `spell_proc_event` WHERE `entry` IN (20335);
INSERT INTO `spell_proc_event` VALUES
 (20335, 0x00, 10, 0x00800000, 0x00000000, 0x00000008, 0x00000100, 0x00000000, 0.000000, 100.000000, 0);
