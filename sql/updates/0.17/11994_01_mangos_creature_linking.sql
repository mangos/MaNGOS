ALTER TABLE db_version CHANGE COLUMN required_11985_01_mangos_gameobject_template_scripts required_11994_01_mangos_creature_linking bit;

--
-- Table structure for table `creature_linking`
--

DROP TABLE IF EXISTS creature_linking;
CREATE TABLE `creature_linking` (
  `guid` int(10) UNSIGNED NOT NULL COMMENT 'creature.guid of the slave mob that is linked',
  `master_guid` int(10) UNSIGNED NOT NULL COMMENT 'master to trigger events',
  `flag` mediumint(8) UNSIGNED NOT NULL COMMENT 'flag - describing what should happen when',
  PRIMARY KEY  (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Creature Linking System';
