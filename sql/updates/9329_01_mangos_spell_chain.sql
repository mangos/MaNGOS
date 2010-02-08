ALTER TABLE db_version CHANGE COLUMN required_9312_01_mangos_quest_template required_9329_01_mangos_spell_chain bit;

/* Pin */
DELETE FROM spell_chain WHERE first_spell = 50519;
INSERT INTO spell_chain VALUES
(50519, 0, 50519, 1, 0),
(53564, 50519, 50519, 2, 0),
(53565, 53564, 50519, 3, 0),
(53566, 53565, 50519, 4, 0),
(53567, 53566, 50519, 5, 0),
(53568, 53567, 50519, 6, 0);

/* Sonic Blast */
DELETE FROM spell_chain WHERE first_spell = 50245;
INSERT INTO spell_chain VALUES
(50245, 0, 50245, 1, 0),
(53544, 50245, 50245, 2, 0),
(53545, 53544, 50245, 3, 0),
(53546, 53545, 50245, 4, 0),
(53547, 53546, 50245, 5, 0),
(53548, 53547, 50245, 6, 0);
