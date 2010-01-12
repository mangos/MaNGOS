ALTER TABLE db_version CHANGE COLUMN required_9150_01_mangos_spell_bonus_data required_9153_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (20167);
