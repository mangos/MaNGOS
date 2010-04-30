ALTER TABLE db_version CHANGE COLUMN required_9477_01_mangos_spell_proc_event required_9482_01_mangos_spell_proc_event bit;

-- (63611) Improved Blood Presence ()
DELETE FROM `spell_proc_event` WHERE `entry` IN (63611);
INSERT INTO `spell_proc_event` VALUES
(63611, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00050014, 0x00000000, 0.000000, 0.000000,  0);
