ALTER TABLE db_version CHANGE COLUMN required_9977_01_mangos_spell_proc_event required_9978_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` IN (55078,55095);
