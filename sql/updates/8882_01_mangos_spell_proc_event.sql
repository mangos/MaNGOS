ALTER TABLE db_version CHANGE COLUMN required_8873_02_mangos_spell_learn_spell required_8882_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (63625);
INSERT INTO `spell_proc_event` VALUES
(63625, 0x00000000, 6, 0x02000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0.000000, 0.000000, 0);

DELETE FROM spell_proc_event WHERE entry IN (56637, 56638);
