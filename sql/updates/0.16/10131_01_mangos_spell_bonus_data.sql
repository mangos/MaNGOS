ALTER TABLE db_version CHANGE COLUMN required_10125_01_mangos_mangos_string required_10131_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` = 54181;

INSERT INTO `spell_bonus_data` VALUES
(54181, 0, 0, 0, 'Warlock - Fel Synergy');