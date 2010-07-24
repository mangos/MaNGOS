ALTER TABLE character_db_version CHANGE COLUMN required_10007_01_characters_pet_aura required_10051_01_characters_character_aura bit;

DELETE FROM character_aura WHERE spell = 58427;
