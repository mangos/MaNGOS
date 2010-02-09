ALTER TABLE character_db_version CHANGE COLUMN required_9246_01_characters_character required_9250_01_characters_character bit;

ALTER TABLE characters
  CHANGE COLUMN `watchedFaction` `watchedFaction` int(10) UNSIGNED NOT NULL default '0';
