ALTER TABLE character_db_version CHANGE COLUMN required_11299_02_characters_pet_aura required_11391_01_characters_auction bit;

ALTER TABLE `auction`
    ADD COLUMN `moneyTime` BIGINT(40) DEFAULT '0' NOT NULL AFTER `time`;
