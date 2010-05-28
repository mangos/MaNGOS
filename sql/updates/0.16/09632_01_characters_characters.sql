ALTER TABLE character_db_version CHANGE COLUMN required_9630_01_characters_characters required_9632_01_characters_characters bit;

ALTER TABLE characters
  ADD COLUMN `knownTitles` longtext AFTER ammoId;
