ALTER TABLE db_version CHANGE COLUMN required_8589_05_mangos_battleground_template required_8589_07_mangos_spell_elixir bit;

DELETE FROM `spell_elixir` WHERE `entry`=67019;

/* Flasks added in 3.2.x */
INSERT INTO `spell_elixir` (`entry`, `mask`) VALUES
(67019,0x3);
