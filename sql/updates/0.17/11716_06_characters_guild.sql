ALTER TABLE character_db_version CHANGE COLUMN required_11716_05_characters_gameobject_respawn required_11716_06_characters_guild bit;

ALTER TABLE `guild`
  CHANGE COLUMN `createdate` `createdate` bigint(20) unsigned NOT NULL default '0',
  CHANGE COLUMN `BankMoney` `BankMoney` bigint(20) unsigned NOT NULL default '0';
