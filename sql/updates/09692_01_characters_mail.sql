ALTER TABLE character_db_version CHANGE COLUMN required_9687_01_characters_character_queststatus_daily required_9692_01_characters_mail bit;

alter table `mail` add column `body` longtext CHARSET utf8 COLLATE utf8_general_ci NULL after `subject`;
