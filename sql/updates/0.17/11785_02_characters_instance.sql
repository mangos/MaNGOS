ALTER TABLE character_db_version CHANGE COLUMN required_11716_10_characters_mail required_11785_02_characters_instance bit;

-- dungeon DBC encounters support
ALTER TABLE `instance` ADD COLUMN `encountersMask` 
INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Dungeon encounter bit mask'
AFTER `difficulty`;