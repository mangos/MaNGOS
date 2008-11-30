ALTER TABLE db_version CHANGE COLUMN required_2008_11_29_01_mangos_spell_proc_event required_2008_11_29_02_mangos_spell_elixir bit;

DELETE FROM `spell_elixir` WHERE `entry` = 45373;
INSERT INTO `spell_elixir` VALUES
(45373,0x1);
