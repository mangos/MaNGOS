ALTER TABLE db_version CHANGE COLUMN required_8840_01_mangos_spell_proc_event required_8841_01_mangos_spell_proc_event bit;

ALTER TABLE `spell_proc_event`
 CHANGE `entry` `entry` mediumint(8) unsigned NOT NULL default '0';

DELETE FROM `spell_proc_event` WHERE `entry` IN (65661);

INSERT INTO `spell_proc_event` VALUES
 (65661, 0, 15, 0x00400010, 0x20020004, 0x00000000, 0x00000010, 0x00000000, 0.000000, 100.000000, 0);
