ALTER TABLE db_version CHANGE COLUMN required_9133_01_mangos_spell_proc_event required_9136_02_mangos_quest_poi bit;

DROP TABLE IF EXISTS `quest_poi`;
CREATE TABLE `quest_poi` (
  `questid` int(11) unsigned NOT NULL DEFAULT '0',
  `objIndex` int(11) NOT NULL DEFAULT '0',
  `mapId` int(11) unsigned NOT NULL DEFAULT '0',
  `unk1` int(11) unsigned NOT NULL DEFAULT '0',
  `unk2` int(11) unsigned NOT NULL DEFAULT '0',
  `unk3` int(11) unsigned NOT NULL DEFAULT '0',
  `unk4` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`questid`,`objIndex`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `quest_poi_points`;
CREATE TABLE `quest_poi_points` (
  `questId` int(11) unsigned NOT NULL DEFAULT '0',
  `objIndex` int(11) NOT NULL DEFAULT '0',
  `x` int(11) NOT NULL DEFAULT '0',
  `y` int(11) NOT NULL DEFAULT '0',
  KEY `idx` (`questId`,`objIndex`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
