ALTER TABLE db_version CHANGE COLUMN required_11501_02_mangos_spell_bonus_data required_11503_01_mangos_spell_proc_event bit;

/*Item - Warrior T10 Melee 2P Bonus*/
DELETE FROM `spell_proc_event` WHERE entry = 70854;
INSERT INTO `spell_proc_event` VALUES
(70854, 0x00,  4, 0x00000000, 0x00000000, 0x00000000, 0x00000010, 0x00000010, 0x00000010, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);
