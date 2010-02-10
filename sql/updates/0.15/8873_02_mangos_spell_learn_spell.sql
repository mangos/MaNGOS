ALTER TABLE db_version CHANGE COLUMN required_8873_01_mangos_spell_proc_event required_8873_02_mangos_spell_learn_spell bit;

DELETE FROM spell_learn_spell WHERE SpellID = 56816;
INSERT INTO spell_learn_spell VALUES (56815, 56816, 0);
