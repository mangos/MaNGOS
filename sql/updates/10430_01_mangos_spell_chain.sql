ALTER TABLE db_version CHANGE COLUMN required_10427_01_mangos_spell_proc_event required_10430_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE first_spell IN (1742, 1784, 5215, 8647, 47476, 50842);
