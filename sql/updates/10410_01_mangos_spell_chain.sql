ALTER TABLE db_version CHANGE COLUMN required_10409_02_mangos_spell_proc_event required_10410_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE spell_id IN (47230, 47231);