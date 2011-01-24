ALTER TABLE db_version CHANGE COLUMN required_10749_01_mangos_mangos_string required_10762_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (52437);
INSERT INTO spell_proc_event VALUES
(52437, 0x00,  4, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000010, 0x00000000, 0.000000, 0.000000,  0);




