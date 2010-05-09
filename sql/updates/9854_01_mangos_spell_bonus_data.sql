ALTER TABLE db_version CHANGE COLUMN required_9826_01_mangos_spell_script_target required_9854_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` = 48743;
INSERT INTO `spell_bonus_data` VALUES (48743, 0, 0, 0, 'Death Knight - Death Pact');