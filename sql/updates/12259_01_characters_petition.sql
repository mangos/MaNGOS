ALTER TABLE character_db_version CHANGE COLUMN required_12161_01_characters_characters required_12259_01_characters_petition bit;

ALTER TABLE `petition` DROP COLUMN `type`;
ALTER TABLE `petition` DROP PRIMARY KEY,
	ADD PRIMARY KEY(`ownerguid`);
