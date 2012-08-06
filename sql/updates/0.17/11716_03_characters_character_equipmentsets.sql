ALTER TABLE character_db_version CHANGE COLUMN required_11716_02_characters_characters required_11716_03_characters_character_equipmentsets bit;

ALTER TABLE `character_equipmentsets`
  CHANGE COLUMN `setguid` `setguid` bigint(20) unsigned NOT NULL auto_increment;
