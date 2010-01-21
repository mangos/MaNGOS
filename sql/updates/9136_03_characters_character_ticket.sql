ALTER TABLE character_db_version CHANGE COLUMN required_9136_01_characters_account_data required_9136_03_characters_character_ticket bit;

alter table `character_ticket`
    add column `response_text` text CHARSET utf8 COLLATE utf8_general_ci NULL after `ticket_text`;
