ALTER TABLE character_db_version CHANGE COLUMN required_9646_01_characters_characters required_9661_01_characters_character_talent bit;

DROP TABLE IF EXISTS `character_talent`;
CREATE TABLE `character_talent` (
  `guid` int(11) unsigned NOT NULL,
  `talent_id` int(11) unsigned NOT NULL,
  `current_rank` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `spec` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`talent_id`,`spec`),
  KEY guid_key (`guid`),
  KEY talent_key (`talent_id`),
  KEY spec_key (`spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
