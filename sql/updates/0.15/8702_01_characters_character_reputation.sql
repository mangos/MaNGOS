ALTER TABLE character_db_version CHANGE COLUMN required_8596_01_characters_bugreport required_8702_01_characters_character_reputation bit;

UPDATE character_reputation SET standing = 0 WHERE faction IN (729, 730) AND standing < 0;
