ALTER TABLE db_version CHANGE COLUMN required_8212_01_mangos_spell_proc_event required_8213_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` where entry='17962';
INSERT INTO `spell_bonus_data` (`entry`) VALUES ('17962');
