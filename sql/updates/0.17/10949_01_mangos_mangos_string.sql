ALTER TABLE db_version CHANGE COLUMN required_10946_01_mangos_spell_proc_event required_10949_01_mangos_mangos_string bit;

UPDATE mangos_string SET content_default='Scripting library not found or not accessible.' WHERE entry=1166;
