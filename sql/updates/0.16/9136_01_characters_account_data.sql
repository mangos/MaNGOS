ALTER TABLE character_db_version CHANGE COLUMN required_8874_01_characters_character_skills required_9136_01_characters_account_data bit;

ALTER table account_data change `data` `data` longblob NOT NULL;
ALTER table character_account_data change `data` `data` longblob NOT NULL;
