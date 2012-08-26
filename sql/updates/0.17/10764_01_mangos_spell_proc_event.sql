ALTER TABLE db_version CHANGE COLUMN required_10762_01_mangos_spell_proc_event required_10764_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (74396);
INSERT INTO spell_proc_event VALUES
(74396, 0x00,  3, 0x28E212F7, 0x28E212F7, 0x28E212F7, 0x00119048, 0x00119048, 0x00119048, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0.000000, 0.000000,  0);
