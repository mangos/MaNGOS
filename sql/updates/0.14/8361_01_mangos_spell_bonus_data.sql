ALTER TABLE db_version CHANGE COLUMN required_8342_01_mangos_spell_proc_event required_8361_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` where entry in (40293);

INSERT INTO `spell_bonus_data` VALUES
(40293, 0, 0, 0, 'Item - Siphon Essence');
