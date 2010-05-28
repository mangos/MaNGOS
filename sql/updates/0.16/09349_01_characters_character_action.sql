ALTER TABLE character_db_version CHANGE COLUMN required_9339_01_characters_group required_9349_01_characters_character_action bit;

ALTER TABLE `character_action` ADD `spec` tinyint(3) unsigned NOT NULL default 0 AFTER `guid`;
