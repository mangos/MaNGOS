ALTER TABLE db_version CHANGE COLUMN required_9460_02_mangos_spell_chain required_9464_01_mangos_spell_proc_event bit;

/*Item - Mage T10 4P Bonus*/
DELETE FROM `spell_proc_event` WHERE `entry` = 70748;
INSERT INTO `spell_proc_event` VALUES
(70748, 0x00000000,  3, 0x00000000, 0x00200000, 0x00000000, 0x00000000, 0x00000000, 0x000000, 0.000000,  0);
