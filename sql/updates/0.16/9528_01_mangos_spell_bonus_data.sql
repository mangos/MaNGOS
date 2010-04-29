ALTER TABLE db_version CHANGE COLUMN required_9526_01_mangos_spell_proc_event required_9528_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (56131);
INSERT INTO spell_bonus_data VALUES
(56131, 0,      0,       0,     'Item - Glyph of Dispel Magic');
