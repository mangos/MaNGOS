ALTER TABLE db_version CHANGE COLUMN required_11613_01_mangos_spell_bonus_data required_11646_01_mangos_item_expire_convert bit;

DROP TABLE IF EXISTS `item_expire_convert`;
CREATE TABLE `item_expire_convert` (
  `entry` mediumint(8) unsigned NOT NULL default '0',
  `item` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Item Convert System';

INSERT INTO `item_expire_convert` VALUES (44623, 44625), (44625, 44627);