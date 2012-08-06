ALTER TABLE db_version CHANGE COLUMN required_10989_01_mangos_loot_template required_10993_01_mangos_loot_template bit;

ALTER TABLE fishing_loot_template
  CHANGE COLUMN entry entry mediumint(8) unsigned NOT NULL default '0' COMMENT 'entry 0 used for junk loot at fishing fail (if allowed by config option)';
