ALTER TABLE db_version CHANGE COLUMN required_0023_xxxxx_01_mangos_player_classlevelstats required_0028_xxxxx_02_mangos_quest_phase_maps bit;

-- ----------------------------
-- Table structure for `quest_phase_maps`
-- ----------------------------
DROP TABLE IF EXISTS `quest_phase_maps`;
CREATE TABLE `quest_phase_maps` (
  `questId` int(11) NOT NULL,
  `map` smallint(6) NOT NULL,
  `phase` int(11) NOT NULL,
  PRIMARY KEY (`questId`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of quest_phase_maps
-- ----------------------------
