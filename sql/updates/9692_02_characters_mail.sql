ALTER TABLE character_db_version CHANGE COLUMN required_9692_01_characters_mail required_9692_02_characters_mail bit;

UPDATE `mail` LEFT JOIN `item_text` ON `mail`.`itemtextid` = `item_text`.`id` SET `mail`.`body`=`item_text`.`text`;
DELETE item_text FROM mail, item_text WHERE mail.itemtextid = item_text.id;
ALTER TABLE `mail` DROP COLUMN `itemtextid`;
