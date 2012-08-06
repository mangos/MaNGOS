ALTER TABLE character_db_version CHANGE COLUMN required_11716_01_characters_auction required_11716_02_characters_characters bit;

ALTER TABLE `characters`
  CHANGE COLUMN `deleteDate` `deleteDate` bigint(20) unsigned default NULL;
