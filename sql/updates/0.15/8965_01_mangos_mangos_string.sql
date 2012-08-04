ALTER TABLE db_version CHANGE COLUMN required_8950_01_mangos_spell_proc_event required_8965_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry in (1015);

INSERT INTO mangos_string VALUES
 (1015,'Can only quit a Remote Admin console or the quit command was not entered in full (quit).',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
