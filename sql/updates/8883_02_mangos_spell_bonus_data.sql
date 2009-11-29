ALTER TABLE db_version CHANGE COLUMN required_8883_01_mangos_spell_proc_event required_8883_02_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 63544;
INSERT INTO spell_bonus_data VALUES
(63544, 0, 0, 0, 'Priest - Empowered Renew Triggered');
