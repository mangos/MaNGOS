ALTER TABLE db_version CHANGE COLUMN required_10906_01_mangos_spell_proc_event required_10906_02_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry=49184;
INSERT INTO spell_bonus_data VALUES
(49184, 0,      0,       0.2,   0,     'Death Knight - Howling Blast');
