ALTER TABLE db_version CHANGE COLUMN required_12112_09_mangos_playercreateinfo required_12112_10_mangos_player_xp_for_level bit;

DELETE FROM `player_xp_for_level` WHERE `lvl` BETWEEN 70 AND 84;

INSERT INTO `player_xp_for_level` VALUES
(70,1219040),
(71,1231680),
(72,1244560),
(73,1257440),
(74,1270320),
(75,1283360),
(76,1296560),
(77,1309920),
(78,1323120),
(79,1336640),
(80,1686300),
(81,2121500),
(82,4004000),
(83,5203400),
(84,9165100);
