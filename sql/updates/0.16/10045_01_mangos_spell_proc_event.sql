ALTER TABLE db_version CHANGE COLUMN required_10044_02_mangos_spell_proc_event required_10045_01_mangos_spell_proc_event bit;

DELETE FROM  `spell_proc_event` WHERE `entry` IN (26016);
