ALTER TABLE character_db_version CHANGE COLUMN required_11716_08_characters_instance required_11716_09_characters_instance_reset bit;

ALTER TABLE `instance_reset`
  CHANGE COLUMN `resettime` `resettime` bigint(40) unsigned NOT NULL default '0';
