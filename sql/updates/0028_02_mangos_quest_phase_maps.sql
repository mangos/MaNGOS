ALTER TABLE db_version CHANGE COLUMN required_0023_xxxxx_01_mangos_player_classlevelstats required_0028_02_mangos_quest_phase_maps bit;

/*
Navicat MySQL Data Transfer

Source Server         : local
Source Server Version : 50519
Source Host           : localhost:3306
Source Database       : mangos

Target Server Type    : MYSQL
Target Server Version : 50519
File Encoding         : 65001

Date: 2012-07-11 19:28:57
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `quest_phase_maps`
-- ----------------------------
DROP TABLE IF EXISTS `quest_phase_maps`;
CREATE TABLE `quest_phase_maps` (
  `questId` int(11) NOT NULL,
  `map` smallint(6) NOT NULL,
  `phase` int(11) NOT NULL,
  PRIMARY KEY (`questId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of quest_phase_maps
-- ----------------------------
