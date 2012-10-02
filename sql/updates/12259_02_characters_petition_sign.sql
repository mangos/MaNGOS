ALTER TABLE character_db_version CHANGE COLUMN required_12259_01_characters_petition required_12259_02_characters_petition_sign bit;

ALTER TABLE `petition_sign` DROP COLUMN `type`;
