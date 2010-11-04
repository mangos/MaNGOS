ALTER TABLE db_version CHANGE COLUMN required_10679_02_mangos_creature_template required_10682_01_mangos_item_convert bit;

DROP TABLE IF EXISTS `item_convert`;
CREATE TABLE `item_convert` (
  `entry` mediumint(8) unsigned NOT NULL default '0',
  `item` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Npc System';
