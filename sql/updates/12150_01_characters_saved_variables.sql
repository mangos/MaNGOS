ALTER TABLE character_db_version CHANGE COLUMN required_12141_01_characters_character_currencies required_12150_01_characters_saved_variables bit;

ALTER TABLE `saved_variables` CHANGE COLUMN `NextArenaPointDistributionTime` `NextCurrenciesResetTime`  bigint(40) unsigned NOT NULL DEFAULT '0';
UPDATE `saved_variables` SET `NextCurrenciesResetTime` = 0;
