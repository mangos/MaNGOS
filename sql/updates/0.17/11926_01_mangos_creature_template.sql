ALTER TABLE db_version CHANGE COLUMN required_11885_01_mangos_spell_proc_event required_11926_01_mangos_creature_template bit;

UPDATE creature_template SET InhabitType=7 WHERE entry=1;
