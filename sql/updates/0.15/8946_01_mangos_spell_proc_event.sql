ALTER TABLE db_version CHANGE COLUMN required_8943_01_mangos_spell_chain required_8946_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (57499);
INSERT INTO `spell_proc_event` VALUES
(57499, 0x00000000,  4, 0x40000001, 0x00010000, 0x00000000, 0x00014000, 0x00000000, 0.000000, 0.000000,0);
