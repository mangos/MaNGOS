ALTER TABLE character_db_version CHANGE COLUMN required_12259_02_characters_petition_sign required_12297_01_characters_auction bit;

ALTER TABLE `auction` MODIFY COLUMN `buyoutprice` bigint(40) unsigned NOT NULL DEFAULT '0';
ALTER TABLE `auction` MODIFY COLUMN `lastbid` bigint(40) unsigned NOT NULL DEFAULT '0';
ALTER TABLE `auction` MODIFY COLUMN `startbid` bigint(40) unsigned NOT NULL DEFAULT '0';
ALTER TABLE `auction` MODIFY COLUMN `deposit` bigint(40) unsigned NOT NULL DEFAULT '0';
ALTER TABLE `auction` MODIFY COLUMN `buyoutprice` bigint(40) unsigned NOT NULL DEFAULT '0';
