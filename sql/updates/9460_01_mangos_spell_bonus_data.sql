ALTER TABLE db_version CHANGE COLUMN required_9450_01_mangos_spell_proc_event required_9460_01_mangos_spell_bonus_data bit;

-- Penance effects (healing bonus 0.537, dmg bonus 0.229)
DELETE FROM spell_bonus_data WHERE entry IN (47666,47750,52983,52984,52985,52998,52999,53000);
INSERT INTO spell_bonus_data VALUES
(47666, 0.229, 0, 0,'Penance - dmg effect'),
(47750, 0.537, 0, 0,'Penance - heal effect');
