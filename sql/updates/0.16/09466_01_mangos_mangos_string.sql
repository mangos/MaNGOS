ALTER TABLE db_version CHANGE COLUMN required_9464_01_mangos_spell_proc_event required_9466_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry=60;
