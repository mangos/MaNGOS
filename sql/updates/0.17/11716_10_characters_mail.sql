ALTER TABLE character_db_version CHANGE COLUMN required_11716_09_characters_instance_reset required_11716_10_characters_mail bit;

ALTER TABLE `mail`
  CHANGE COLUMN `expire_time` `expire_time` bigint(40) unsigned NOT NULL default '0',
  CHANGE COLUMN `deliver_time` `deliver_time` bigint(40) unsigned NOT NULL default '0';

