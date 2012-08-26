ALTER TABLE character_db_version CHANGE COLUMN required_11620_01_characters_character_equipmentsets required_11704_01_characters_auction bit;

ALTER TABLE `auction`
  DROP KEY `item_guid`,
  ADD COLUMN `item_count` int(11) unsigned NOT NULL default '0' AFTER `item_template`,
  ADD COLUMN `item_randompropertyid` int(11) NOT NULL default '0' AFTER `item_count`;

UPDATE auction, item_instance
  SET auction.item_count = SUBSTRING_INDEX(SUBSTRING_INDEX(item_instance.data, ' ', 14 + 1), ' ', -1)
  WHERE auction.itemguid = item_instance.guid;
