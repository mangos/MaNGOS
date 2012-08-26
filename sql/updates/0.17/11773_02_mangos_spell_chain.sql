ALTER TABLE db_version CHANGE COLUMN required_11773_01_mangos_spell_proc_event required_11773_02_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE spell_id IN (53672, 54149);
INSERT INTO spell_chain VALUES
(53672,0,53672,1,0),
(54149,53672,53672,2,0);