ALTER TABLE db_version CHANGE COLUMN required_10660_01_mangos_game_event_quest required_10679_01_mangos_npc_vendor_template bit;

DROP TABLE IF EXISTS `npc_vendor_template`;
CREATE TABLE `npc_vendor_template` (
  `entry` mediumint(8) unsigned NOT NULL default '0',
  `item` mediumint(8) unsigned NOT NULL default '0',
  `maxcount` tinyint(3) unsigned NOT NULL default '0',
  `incrtime` int(10) unsigned NOT NULL default '0',
  `ExtendedCost` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`entry`,`item`,`ExtendedCost`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Npc System';
