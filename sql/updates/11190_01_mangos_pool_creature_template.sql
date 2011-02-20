ALTER TABLE db_version CHANGE COLUMN required_11169_02_mangos_command required_11190_01_mangos_pool_creature_template bit;

DROP TABLE IF EXISTS `pool_creature_template`;
CREATE TABLE `pool_creature_template` (
  `id` int(10) unsigned NOT NULL default '0',
  `pool_entry` mediumint(8) unsigned NOT NULL default '0',
  `chance` float unsigned NOT NULL default '0',
  `description` varchar(255) NOT NULL,
  PRIMARY KEY  (`id`),
  INDEX `pool_idx` (pool_entry)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;


