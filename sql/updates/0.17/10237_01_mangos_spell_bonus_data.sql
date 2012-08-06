ALTER TABLE db_version CHANGE COLUMN required_10223_01_mangos_spell_proc_event required_10237_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 10444;
INSERT INTO spell_bonus_data VALUES (10444, 0, 0, 0, "Shaman - Flametongue Attack");
