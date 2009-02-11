ALTER TABLE character_db_version CHANGE COLUMN required_7207_03_characters_corpse required_7251_02_characters_character_spell bit;

DELETE FROM `character_spell` WHERE `spell` IN (52375,47541);
