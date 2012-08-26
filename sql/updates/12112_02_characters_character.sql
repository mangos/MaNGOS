ALTER TABLE character_db_version CHANGE COLUMN required_12112_01_characters_character_phase_data required_12112_02_characters_character bit;

ALTER TABLE `characters` ADD COLUMN `slot` tinyint(3) unsigned NOT NULL DEFAULT '255' AFTER `actionBars`;
