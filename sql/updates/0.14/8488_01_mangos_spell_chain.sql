ALTER TABLE db_version CHANGE COLUMN required_8487_02_mangos_spell_proc_event required_8488_01_mangos_spell_chain bit;

DELETE FROM `spell_chain` WHERE `spell_id` IN (27813, 27817, 27818, 61391, 61390, 61388, 61387, 53227, 47960, 61291);

INSERT INTO `spell_chain` (`spell_id`, `prev_spell`, `first_spell`, `rank`, `req_spell`) VALUES
/*Blessed Recovery Proc*/
('27813', '0', '27813', '1', '0'),
('27817', '27813', '27813', '2', '0'),
('27818', '27817', '27813', '3', '0'),
/*Typhoon Triggered*/
('61391', '0', '61391', '1', '0'),
('61390', '61391', '61391', '2', '0'),
('61388', '61390', '61391', '3', '0'),
('61387', '61388', '61391', '4', '0'),
('53227', '61387', '61391', '5', '0'),
/*Shadowflame Triggered DoT*/
('47960','0','47960','1','0'),
('61291','47960','47960','2','0');
