ALTER TABLE db_version CHANGE COLUMN required_10205_01_mangos_spell_area required_10207_01_mangos_npc_vendor bit;

UPDATE npc_vendor
  SET ExtendedCost = abs(ExtendedCost) WHERE ExtendedCost < 0;

ALTER TABLE npc_vendor
  CHANGE COLUMN `ExtendedCost` `ExtendedCost` mediumint(8) unsigned NOT NULL default '0';
