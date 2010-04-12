ALTER TABLE db_version CHANGE COLUMN required_9735_01_mangos_spell_proc_event required_9735_02_mangos_spell_chain bit;

DELETE FROM  `spell_chain` WHERE `spell_id` IN (53709, 53710, 53711);
INSERT INTO `spell_chain` (`spell_id`, `prev_spell`, `first_spell`, `rank`, `req_spell`) VALUES
('53709', '0', '53709', '1', '0'),
('53710', '53709', '53709','2', '0'),
('53711', '53710', '53709', '3', '0');