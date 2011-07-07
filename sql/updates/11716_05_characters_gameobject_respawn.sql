ALTER TABLE character_db_version CHANGE COLUMN required_11716_04_characters_creature_respawn required_11716_05_characters_gameobject_respawn bit;

ALTER TABLE `gameobject_respawn`
  CHANGE COLUMN `respawntime` `respawntime` bigint(20) unsigned NOT NULL default '0';
