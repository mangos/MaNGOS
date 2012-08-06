ALTER TABLE character_db_version CHANGE COLUMN required_10503_02_characters_gameobject_respawn required_10568_01_characters_character_tutorial bit;

ALTER TABLE `character_tutorial` DROP PRIMARY KEY;
ALTER TABLE `character_tutorial` DROP COLUMN `realmid`;
ALTER TABLE `character_tutorial` ADD PRIMARY KEY (`account`);
ALTER TABLE `character_tutorial` DROP KEY `acc_key`;
