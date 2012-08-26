ALTER TABLE character_db_version CHANGE COLUMN required_11716_03_characters_character_equipmentsets required_11716_04_characters_creature_respawn bit;

ALTER TABLE `creature_respawn`
  CHANGE COLUMN `respawntime` `respawntime` bigint(20) unsigned NOT NULL default '0';
