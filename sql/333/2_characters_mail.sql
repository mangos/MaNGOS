UPDATE `mail` LEFT JOIN `item_text` ON `mail`.`itemtextid` = `item_text`.`id` SET `mail`.`body`=`item_text`.`text`;
DELETE item_text FROM mail, item_text WHERE mail.itemtextid = item_text.id;
ALTER TABLE `mail` DROP COLUMN `itemtextid`;
