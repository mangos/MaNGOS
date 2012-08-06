ALTER TABLE db_version CHANGE COLUMN required_10353_02_mangos_command required_10362_01_mangos_creature_movement_template bit;

DROP TABLE IF EXISTS `creature_movement_template`;
CREATE TABLE `creature_movement_template` (
  `entry` mediumint(8) unsigned NOT NULL COMMENT 'Creature entry',
  `point` mediumint(8) unsigned NOT NULL default '0',
  `position_x` float NOT NULL default '0',
  `position_y` float NOT NULL default '0',
  `position_z` float NOT NULL default '0',
  `waittime` int(10) unsigned NOT NULL default '0',
  `script_id` mediumint(8) unsigned NOT NULL default '0',
  `textid1` int(11) NOT NULL default '0',
  `textid2` int(11) NOT NULL default '0',
  `textid3` int(11) NOT NULL default '0',
  `textid4` int(11) NOT NULL default '0',
  `textid5` int(11) NOT NULL default '0',
  `emote` mediumint(8) unsigned NOT NULL default '0',
  `spell` mediumint(8) unsigned NOT NULL default '0',
  `wpguid` int(11) NOT NULL default '0',
  `orientation` float NOT NULL default '0',
  `model1` mediumint(9) NOT NULL default '0',
  `model2` mediumint(9) NOT NULL default '0',
  PRIMARY KEY  (`entry`,`point`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Creature waypoint system';
