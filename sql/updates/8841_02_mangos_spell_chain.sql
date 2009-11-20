ALTER TABLE db_version CHANGE COLUMN required_8841_01_mangos_spell_proc_event required_8841_02_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE first_spell = 65661;

INSERT INTO spell_chain VALUES
(65661,0,65661,1,0),
(66191,65661,65661,2,0),
(66192,66191,65661,3,0);
