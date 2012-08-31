ALTER TABLE character_db_version CHANGE COLUMN required_12150_01_characters_saved_variables required_12161_01_characters_characters bit;

ALTER TABLE `characters` ADD `primary_trees` varchar(10) NOT NULL DEFAULT '0 0 ' AFTER `resettalents_time`;
