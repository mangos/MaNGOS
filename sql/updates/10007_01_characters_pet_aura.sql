ALTER TABLE character_db_version CHANGE COLUMN required_9974_01_characters_group required_10007_01_characters_pet_aura bit;

UPDATE `pet_aura` SET remaincharges = 0 WHERE remaincharges = 255;