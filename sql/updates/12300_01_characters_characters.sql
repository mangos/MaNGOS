ALTER TABLE character_db_version CHANGE COLUMN required_12297_01_characters_auction required_12300_01_characters_characters bit;

ALTER TABLE `characters` MODIFY COLUMN `money` bigint(40) unsigned NOT NULL DEFAULT '0';
