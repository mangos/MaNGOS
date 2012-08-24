ALTER TABLE db_version CHANGE COLUMN required_0168_xxxxx_01_mangos_playercreateinfo_spell required_0169_xxxxx_01_mangos_npc_vendor bit;

ALTER TABLE `npc_vendor` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `npc_vendor` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL default '0';
