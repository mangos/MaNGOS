ALTER TABLE character_db_version CHANGE COLUMN required_12300_01_characters_characters required_12300_02_characters_mail bit;

ALTER TABLE `mail` MODIFY COLUMN `money` bigint(40) unsigned NOT NULL DEFAULT '0';
ALTER TABLE `mail` MODIFY COLUMN `cod` bigint(40) unsigned NOT NULL DEFAULT '0';
