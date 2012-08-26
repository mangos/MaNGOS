ALTER TABLE character_db_version CHANGE COLUMN required_11785_02_characters_instance required_12112_01_characters_character_phase_data bit;

DROP TABLE IF EXISTS `character_phase_data`;
CREATE TABLE `character_phase_data` (
  `guid` int(11) NOT NULL,
  `map` smallint(6) NOT NULL,
  `phase` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
