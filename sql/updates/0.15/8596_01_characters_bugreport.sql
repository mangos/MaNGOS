ALTER TABLE character_db_version CHANGE COLUMN required_8589_11_characters_characters required_8596_01_characters_bugreport bit;

ALTER TABLE `bugreport` CHANGE `type` `type` LONGTEXT NOT NULL;
ALTER TABLE `bugreport` CHANGE `content` `content` LONGTEXT NOT NULL;
