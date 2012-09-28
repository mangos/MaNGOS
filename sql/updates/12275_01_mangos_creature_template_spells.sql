ALTER TABLE db_version CHANGE COLUMN required_12274_01_mangos_vehicle_accessory required_12275_01_mangos_creature_template_spells bit;

--
-- Table structure for table `creature_template_spells`
--
DROP TABLE IF EXISTS `creature_template_spells`;
CREATE TABLE `creature_template_spells` (
  `entry` mediumint(8) unsigned NOT NULL,
  `spell1` mediumint(8) unsigned NOT NULL,
  `spell2` mediumint(8) unsigned NOT NULL default '0',
  `spell3` mediumint(8) unsigned NOT NULL default '0',
  `spell4` mediumint(8) unsigned NOT NULL default '0',
  `spell5` mediumint(8) unsigned NOT NULL default '0',
  `spell6` mediumint(8) unsigned NOT NULL default '0',
  `spell7` mediumint(8) unsigned NOT NULL default '0',
  `spell8` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Creature System (Spells used by creature)';

--
-- Dumping data for table `creature_template_spells`
--
INSERT INTO creature_template_spells (entry, spell1, spell2, spell3, spell4) SELECT entry, spell1, spell2, spell3, spell4 FROM creature_template WHERE spell1!=0;
