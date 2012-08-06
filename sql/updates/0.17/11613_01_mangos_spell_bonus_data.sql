ALTER TABLE db_version CHANGE COLUMN required_11606_01_mangos_spell_proc_event required_11613_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry IN (50288,50294);
INSERT INTO spell_bonus_data VALUES
(50288, 0.3,    0,       0,     0,     'Druid - Starfall'),
(50294, 0.13,   0,       0,     0,     'Druid - Starfall AOE');
