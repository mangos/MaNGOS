ALTER TABLE character_db_version CHANGE COLUMN required_10973_01_characters_game_event_status required_11117_02_characters_world bit;

--
-- Table structure for table `world`
--

DROP TABLE IF EXISTS `world`;
CREATE TABLE `world` (
  `map` int(11) unsigned NOT NULL default '0',
  `data` longtext,
  PRIMARY KEY  (`map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
