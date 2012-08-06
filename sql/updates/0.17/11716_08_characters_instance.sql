ALTER TABLE character_db_version CHANGE COLUMN required_11716_07_characters_guild_eventlog required_11716_08_characters_instance bit;

ALTER TABLE `instance`
  CHANGE COLUMN `resettime` `resettime` bigint(40) unsigned NOT NULL default '0';
