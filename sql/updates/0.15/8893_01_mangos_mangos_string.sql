ALTER TABLE db_version CHANGE COLUMN required_8891_01_mangos_spell_proc_event required_8893_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry in (355);

INSERT INTO mangos_string VALUES
 (355,'Title %u (%s) set as current selected title for player %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
