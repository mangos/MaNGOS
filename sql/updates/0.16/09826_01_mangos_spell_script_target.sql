ALTER TABLE db_version CHANGE COLUMN required_9803_01_mangos_spell_bonus_data required_9826_01_mangos_spell_script_target bit;

DELETE FROM `spell_script_target` WHERE `entry` IN ('38736','38729');
INSERT INTO `spell_script_target` VALUES ('38736','1','22288'), ('38729','0','185191');