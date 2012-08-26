ALTER TABLE db_version CHANGE COLUMN required_11968_01_mangos_creature_linking_template required_11985_01_mangos_gameobject_template_scripts bit;

--
-- Table structure for table `gameobject_template_scripts`
--

DROP TABLE IF EXISTS `gameobject_template_scripts`;
CREATE TABLE `gameobject_template_scripts` (
  `id` mediumint(8) unsigned NOT NULL default '0',
  `delay` int(10) unsigned NOT NULL default '0',
  `command` mediumint(8) unsigned NOT NULL default '0',
  `datalong` mediumint(8) unsigned NOT NULL default '0',
  `datalong2` int(10) unsigned NOT NULL default '0',
  `buddy_entry` int(10) unsigned NOT NULL default '0',
  `search_radius` int(10) unsigned NOT NULL default '0',
  `data_flags` tinyint(3) unsigned NOT NULL default '0',
  `dataint` int(11) NOT NULL default '0',
  `dataint2` int(11) NOT NULL default '0',
  `dataint3` int(11) NOT NULL default '0',
  `dataint4` int(11) NOT NULL default '0',
  `x` float NOT NULL default '0',
  `y` float NOT NULL default '0',
  `z` float NOT NULL default '0',
  `o` float NOT NULL default '0',
  `comments` varchar(255) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Dumping data for table `gameobject_template_scripts`
--

LOCK TABLES `gameobject_template_scripts` WRITE;
/*!40000 ALTER TABLE `gameobject_template_scripts` DISABLE KEYS */;
/*!40000 ALTER TABLE `gameobject_template_scripts` ENABLE KEYS */;
UNLOCK TABLES;
