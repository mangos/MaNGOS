ALTER TABLE db_version CHANGE COLUMN required_11501_01_mangos_spell_proc_event required_11501_02_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (64569);
INSERT INTO spell_bonus_data VALUES
(64569, 0,  0,       0,     0,     'Item - Blood Reserve');
