ALTER TABLE character_db_version CHANGE COLUMN required_8589_04_characters_groups required_8589_06_characters_bugreport bit;

ALTER TABLE `bugreport` CHANGE `content` `content` LONGTEXT;
ALTER TABLE `bugreport` CHANGE `type` `type` LONGTEXT;
