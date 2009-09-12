ALTER TABLE db_version CHANGE COLUMN required_8487_01_mangos_spell_bonus_data required_8487_02_mangos_spell_proc_event bit;

DELETE FROM `spell_proc_event` WHERE `entry` IN (31801, 53736);
