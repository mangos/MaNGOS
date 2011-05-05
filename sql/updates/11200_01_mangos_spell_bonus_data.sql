ALTER TABLE db_version CHANGE COLUMN required_11190_01_mangos_pool_gameobject_template required_11200_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (1776, 8680, 13218);
INSERT INTO spell_bonus_data VALUES
(1776,  0, 0, 0.21, 0, 'Rogue - Gouge'),
(8680,  0, 0, 0.10, 0, 'Rogue - Instant Poison'),
(13218, 0, 0, 0.04, 0, 'Rogue - Wound Poison');
