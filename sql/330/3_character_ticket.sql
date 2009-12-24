alter table `character_ticket`
    add column `response_text` text CHARSET utf8 COLLATE utf8_general_ci NULL after `ticket_text`;
