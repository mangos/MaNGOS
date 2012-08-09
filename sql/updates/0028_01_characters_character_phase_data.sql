ALTER TABLE db_version CHANGE COLUMN required_0001_xxxxx_01_characters required_0028_01_characters_character_phase_data bit;

/*
Navicat MySQL Data Transfer

Source Server         : local
Source Server Version : 50519
Source Host           : localhost:3306
Source Database       : chars

Target Server Type    : MYSQL
Target Server Version : 50519
File Encoding         : 65001

Date: 2012-07-11 19:50:47
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `character_phase_data`
-- ----------------------------
DROP TABLE IF EXISTS `character_phase_data`;
CREATE TABLE `character_phase_data` (
  `guid` int(11) NOT NULL,
  `map` smallint(6) NOT NULL,
  `phase` int(11) NOT NULL DEFAULT '1',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of character_phase_data
-- ----------------------------
