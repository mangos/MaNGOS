ALTER TABLE db_version CHANGE COLUMN required_0169_xxxxx_01_mangos_npc_vendor required_0169_xxxxx_02_mangos_npc_vendor_template bit;

ALTER TABLE `npc_vendor_template` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `npc_vendor_template` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL default '0';