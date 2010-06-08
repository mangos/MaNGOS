ALTER TABLE db_version CHANGE COLUMN required_9136_05_mangos_spell_bonus_data required_9136_06_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (55668,55669,55670,55667,58631,63320);


INSERT INTO `spell_proc_event` VALUES
(63320, 0x00000000,  5, 0x00040000, 0x00000000, 0x00008000, 0x00004000, 0x00000001, 0.000000, 0.000000,  0);
