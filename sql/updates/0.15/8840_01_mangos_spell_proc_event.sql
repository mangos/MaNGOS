ALTER TABLE db_version CHANGE COLUMN required_8835_01_mangos_command required_8840_01_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry`=53601;

INSERT INTO spell_proc_event VALUES
(53601, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x000A02A8, 0x00000000, 0.000000, 0.000000,  6);
