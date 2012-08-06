ALTER TABLE db_version CHANGE COLUMN required_10764_01_mangos_spell_proc_event required_10786_01_mangos_spell_proc_item_enchant bit;

DELETE FROM spell_proc_item_enchant  WHERE entry IN (13897, 20004, 20005, 44525, 44578);
INSERT INTO spell_proc_item_enchant VALUES
(13897, 6.0), -- Enchant Weapon - Fiery Weapon
(20004, 6.0), -- Enchant Weapon - Lifestealing
(20005, 1.6), -- Enchant Weapon - Icy Chill
(44525, 3.4), -- Enchant Weapon - Icebreaker
(44578, 3.4); -- Enchant Weapon - Lifeward
