ALTER TABLE db_version CHANGE COLUMN required_9528_01_mangos_spell_bonus_data required_9539_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (46567,54757);
INSERT INTO spell_bonus_data VALUES
(46567, 0,      0,       0,     'Item - Goblin Rocket Launcher'),
(54757, 0,      0,       0,     'Generic - Pyro Rocket');
