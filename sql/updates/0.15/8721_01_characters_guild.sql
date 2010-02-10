ALTER TABLE character_db_version CHANGE COLUMN required_8702_01_characters_character_reputation required_8721_01_characters_guild bit;

UPDATE guild_rank SET BankMoneyPerDay = 4294967295 WHERE rid = 0;