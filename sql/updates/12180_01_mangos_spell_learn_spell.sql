ALTER TABLE db_version CHANGE COLUMN required_12179_01_mangos_player_levelstats required_12180_01_mangos_spell_learn_spell bit;

ALTER TABLE `spell_learn_spell` MODIFY COLUMN `entry` mediumint(8) unsigned NOT NULL DEFAULT '0';
ALTER TABLE `spell_learn_spell` MODIFY COLUMN `SpellID` mediumint(8) unsigned NOT NULL DEFAULT '0';

DELETE FROM `spell_learn_spell` WHERE `entry` IN
(87492, 87491, 87493, 86467, 87494, 87495, 87496, 87497, 87498, 87500);

INSERT INTO `spell_learn_spell` (`entry`, `SpellID`, `Active`) VALUES
-- Death Knight
(87492, 86471, 1),
-- Druid
(87491, 86470, 1),
-- Hunter
(87493, 86472, 1),
-- Mage
(86467, 86473, 1),
-- Paladin
(87494, 86474, 1),
-- Priest
(87495, 86475, 1),
-- Rogue
(87496, 86476, 1),
-- Shaman
(87497, 86477, 1),
-- Warlock
(87498, 86478, 1),
-- Warrior
(87500, 86479, 1);
