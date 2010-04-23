ALTER TABLE db_version CHANGE COLUMN required_8548_01_mangos_creature_battleground required_8548_02_mangos_gameobject_battleground bit;

DROP TABLE IF EXISTS `gameobject_battleground`;
CREATE TABLE `gameobject_battleground` (
    `guid` int(10) unsigned NOT NULL COMMENT 'GameObject\'s GUID',
    `event1` tinyint(3) unsigned NOT NULL COMMENT 'main event',
    `event2` tinyint(3) unsigned NOT NULL COMMENT 'sub event',
    PRIMARY KEY  (`guid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='GameObject battleground indexing system';
