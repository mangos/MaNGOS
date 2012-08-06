ALTER TABLE character_db_version CHANGE COLUMN required_11716_06_characters_guild required_11716_07_characters_guild_eventlog bit;

ALTER TABLE `guild_eventlog`
  CHANGE COLUMN `guildid` `guildid` int(11) unsigned NOT NULL COMMENT 'Guild Identificator',
  CHANGE COLUMN `LogGuid` `LogGuid` int(11) unsigned NOT NULL COMMENT 'Log record identificator - auxiliary column',
  CHANGE COLUMN `EventType` `EventType` tinyint(1) unsigned NOT NULL COMMENT 'Event type',
  CHANGE COLUMN `PlayerGuid1` `PlayerGuid1` int(11) unsigned NOT NULL COMMENT 'Player 1',
  CHANGE COLUMN `PlayerGuid2` `PlayerGuid2` int(11) unsigned NOT NULL COMMENT 'Player 2',
  CHANGE COLUMN `NewRank` `NewRank` tinyint(2) unsigned NOT NULL COMMENT 'New rank(in case promotion/demotion)',
  CHANGE COLUMN `TimeStamp` `TimeStamp` bigint(20) unsigned NOT NULL COMMENT 'Event UNIX time';
