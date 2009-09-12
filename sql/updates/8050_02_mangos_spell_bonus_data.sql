ALTER TABLE db_version CHANGE COLUMN required_8050_01_mangos_spell_proc_event required_8050_02_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 63106;
INSERT INTO spell_bonus_data VALUES (63106, 0, 0, 0, 'Warlock - Siphon Life Triggered');
