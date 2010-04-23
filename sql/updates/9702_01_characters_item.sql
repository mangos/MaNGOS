ALTER TABLE character_db_version CHANGE COLUMN required_9692_02_characters_mail required_9702_01_characters_item bit;


ALTER TABLE `item_instance`
  ADD COLUMN `text` longtext AFTER `data`;

-- indexes in any case broken and lost after 3.3.3 switch.
DROP TABLE IF EXISTS `item_text`;
