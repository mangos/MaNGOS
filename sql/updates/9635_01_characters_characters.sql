ALTER TABLE character_db_version CHANGE COLUMN required_9634_01_characters_corpse required_9635_01_characters_characters bit;

ALTER TABLE characters
  ADD COLUMN `actionBars` tinyint(3) UNSIGNED NOT NULL default '0' AFTER knownTitles;
