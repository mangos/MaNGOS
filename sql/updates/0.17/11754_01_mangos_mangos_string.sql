ALTER TABLE db_version CHANGE COLUMN required_11733_01_mangos_spell_proc_event required_11754_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1192);

INSERT INTO mangos_string VALUES (1192,'Effect movement',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
