ALTER TABLE db_version CHANGE COLUMN required_8882_02_mangos_spell_chain required_8882_03_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 63675;
INSERT INTO spell_bonus_data VALUES
(63675, 0, 0, 0, 'Priest - Improved Devouring Plague Triggered');
