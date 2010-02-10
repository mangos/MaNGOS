ALTER TABLE character_db_version CHANGE COLUMN required_8721_01_characters_guild required_8828_01_characters_instance_reset bit;

ALTER TABLE instance_reset
  ADD COLUMN difficulty tinyint(1) unsigned NOT NULL default '0' AFTER mapid,
  DROP PRIMARY KEY,
  ADD  PRIMARY KEY  (`mapid`,`difficulty`);
