ALTER TABLE db_version CHANGE COLUMN required_8930_01_mangos_spell_proc_event required_8931_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 172;
INSERT INTO spell_bonus_data  (entry, direct_bonus, dot_bonus, ap_bonus, comments) VALUES
(172, 0, 0.2, 0, 'Warlock - Corruption');
