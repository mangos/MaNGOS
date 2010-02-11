ALTER TABLE character_db_version CHANGE COLUMN required_9349_01_characters_character_action required_9354_01_characters_character_action bit;

ALTER TABLE `character_action` DROP PRIMARY KEY, ADD PRIMARY KEY(`guid`,`spec`,`button`);
