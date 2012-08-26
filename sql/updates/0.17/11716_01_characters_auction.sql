ALTER TABLE character_db_version CHANGE COLUMN required_11704_01_characters_auction required_11716_01_characters_auction bit;

ALTER TABLE `auction`
  CHANGE COLUMN `time` `time` bigint(40) unsigned NOT NULL default '0',
  CHANGE COLUMN `moneyTime` `moneyTime` bigint(40) unsigned NOT NULL default '0';
