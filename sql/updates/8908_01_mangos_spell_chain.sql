ALTER TABLE db_version CHANGE COLUMN required_8899_01_mangos_spell_proc_event required_8908_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE spell_id IN (27681,32999,48074);
INSERT INTO spell_chain VALUES
(27681,14752,14752,2,0),
(32999,27681,14752,3,0),
(48074,32999,14752,4,0);
