ALTER TABLE db_version CHANGE COLUMN required_8965_02_mangos_command required_8980_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 56160;
INSERT INTO spell_bonus_data  (entry, direct_bonus, dot_bonus, ap_bonus, comments) VALUES
(56160, 0, 0, 0, 'Item - Glyph of Power Word: Shield');
