ALTER TABLE db_version CHANGE COLUMN required_9957_01_mangos_mangos_string required_9957_02_mangos_npc_vendor bit;

ALTER TABLE npc_vendor
  CHANGE COLUMN `ExtendedCost` `ExtendedCost` mediumint(8) NOT NULL default '0' COMMENT 'negative if cost must exclude normal money cost';
