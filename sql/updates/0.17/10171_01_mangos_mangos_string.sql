ALTER TABLE db_version CHANGE COLUMN required_10159_01_mangos_spell_proc_event required_10171_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (27,1029);

INSERT INTO mangos_string VALUES
(27,'The old password is wrong',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1029, 'Command can be called only from RA-console.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
