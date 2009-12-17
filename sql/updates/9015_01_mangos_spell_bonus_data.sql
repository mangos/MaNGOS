ALTER TABLE db_version CHANGE COLUMN required_9007_01_mangos_spell_proc_event required_9015_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 64085;
INSERT INTO spell_bonus_data  (entry, direct_bonus, dot_bonus, ap_bonus, comments) VALUES
(64085, 0, 0, 0, 'Priest - Vampiric Touch Dispel');
