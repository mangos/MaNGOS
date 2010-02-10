ALTER TABLE db_version CHANGE COLUMN required_9045_01_mangos_spell_proc_event required_9045_02_mangos_spell_chain bit;

/*Revitalize*/
DELETE FROM spell_chain WHERE first_spell = 48539;
INSERT INTO spell_chain VALUES
(48539, 0, 48539, 1, 0),
(48544, 48539, 48539, 2, 0),
(48545, 48544, 48539, 3, 0);
