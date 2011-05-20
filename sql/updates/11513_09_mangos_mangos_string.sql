ALTER TABLE db_version CHANGE COLUMN required_11503_01_mangos_spell_proc_event required_11513_09_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (8);
INSERT INTO mangos_string VALUES
(8,'Command %s have subcommands:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
