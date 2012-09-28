ALTER TABLE db_version CHANGE COLUMN required_12245_01_mangos_mangos_string required_12257_01_mangos_spell_learn_spell bit;

DELETE FROM `spell_learn_spell` WHERE `entry` IN (86524, 86525, 86526, 86528, 86529, 86530, 86531);
INSERT INTO `spell_learn_spell` (`entry`, `SpellID`, `Active`) VALUES
-- dk
(86524, 86113, 1),
(86524, 86536, 1),
(86524, 86537, 1),
-- pally
(86525, 86102, 1),
(86525, 86103, 1),
(86525, 86539, 1),
-- warr
(86526, 86101, 1),
(86526, 86110, 1),
(86526, 86535, 1),
-- hunter
(86528, 86538, 1),
-- shaman
(86529, 86099, 1),
(86529, 86100, 1),
(86529, 86108, 1),
-- druid
(86530, 86093, 1),
(86530, 86096, 1),
(86530, 86097, 1),
(86530, 86104, 1),
-- rogue
(86531, 86092, 1);