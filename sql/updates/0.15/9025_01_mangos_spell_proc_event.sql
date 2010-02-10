ALTER TABLE db_version CHANGE COLUMN required_9019_01_mangos_spell_threat required_9025_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` = 63373;
INSERT INTO `spell_proc_event` VALUES
(63373, 0x00000000,11, 0x80000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0.000000, 0.000000,  0);
