ALTER TABLE db_version CHANGE COLUMN required_9222_01_mangos_playercreateinfo_spell required_9244_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (49188,56822,59057);
INSERT INTO `spell_proc_event` VALUES
(49188, 0x00000000, 15, 0x00000000, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);
