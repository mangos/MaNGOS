ALTER TABLE db_version CHANGE COLUMN required_9125_01_mangos_npc_spellclick_spells required_9133_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (11129);
INSERT INTO `spell_proc_event` VALUES
(11129, 0x00000004,  3, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);
