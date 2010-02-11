ALTER TABLE character_db_version CHANGE COLUMN required_9354_01_characters_character_action required_9359_01_characters_characters bit;

ALTER TABLE `characters` ADD `specCount` tinyint(3) unsigned NOT NULL default 1 AFTER `power7`;
ALTER TABLE `characters` ADD `activeSpec` tinyint(3) unsigned NOT NULL default 0 AFTER `specCount`;
