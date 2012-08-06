ALTER TABLE db_version CHANGE COLUMN required_11433_01_mangos_item_template required_11452_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (15662, 11538, 21179);
INSERT INTO spell_bonus_data VALUES
(15662, 0,  0,       0,     0,     'Item - Six Demon Bag - Fireball'),
(11538, 0,  0,       0,     0,     'Item - Six Demon Bag - Frostbolt'),
(21179, 0,  0,       0,     0,     'Item - Six Demon Bag - Chain Lightning');
