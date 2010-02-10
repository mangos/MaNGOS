ALTER TABLE db_version CHANGE COLUMN required_8929_01_mangos_gossip_scripts required_8930_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (58597);
INSERT INTO `spell_proc_event` VALUES
(58597, 0x00000000, 10, 0x40000000, 0x00000000, 0x00000000, 0x00008000, 0x00000000, 0.000000, 100.000000,0);
