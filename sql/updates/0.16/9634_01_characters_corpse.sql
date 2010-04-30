ALTER TABLE character_db_version CHANGE COLUMN required_9632_01_characters_characters required_9634_01_characters_corpse bit;

ALTER TABLE corpse
  DROP COLUMN data,
  DROP COLUMN zone;
