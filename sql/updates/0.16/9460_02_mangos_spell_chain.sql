ALTER TABLE db_version CHANGE COLUMN required_9460_01_mangos_spell_bonus_data required_9460_02_mangos_spell_chain bit;

-- Penance (damage)
DELETE FROM spell_chain WHERE first_spell = 47666;
INSERT INTO spell_chain VALUES
(47666, 0, 47666, 1, 0),
(52998, 47666, 47666, 2, 0),
(52999, 52998, 47666, 3, 0),
(53000, 52999, 47666, 4, 0);
-- Penance (healing)
DELETE FROM spell_chain WHERE first_spell = 47750;
INSERT INTO spell_chain VALUES
(47750, 0, 47750, 1, 0),
(52983, 47750, 47750, 2, 0),
(52984, 52983, 47750, 3, 0),
(52985, 52984, 47750, 4, 0);
