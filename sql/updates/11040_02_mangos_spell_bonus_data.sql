ALTER TABLE db_version CHANGE COLUMN required_11040_01_mangos_spell_chain required_11040_02_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (52042, 5672);
INSERT INTO spell_bonus_data VALUES
(5672, 0.08272,  0,       0,     0,     'Shaman - Healing Stream Totem Aura');
