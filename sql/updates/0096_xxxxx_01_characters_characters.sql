ALTER TABLE db_version CHANGE COLUMN required_0028_xxxxx_01_characters_character_phase_data required_0096_xxxxx_01_characters_characters bit;

ALTER TABLE `characters` ADD COLUMN `slot` tinyint(3) unsigned NOT NULL DEFAULT '255' AFTER `actionBars`;
