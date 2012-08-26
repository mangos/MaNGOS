ALTER TABLE db_version CHANGE COLUMN required_12112_06_mangos_mangos_string required_12112_07_mangos_quest_phase_maps bit;

DROP TABLE IF EXISTS `quest_phase_maps`;
CREATE TABLE `quest_phase_maps` (
  `questId` int(11) NOT NULL,
  `map` smallint(6) NOT NULL,
  `phase` int(11) NOT NULL,
  PRIMARY KEY (`questId`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
