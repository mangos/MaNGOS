ALTER TABLE db_version CHANGE COLUMN required_12216_01_mangos_creature_loot_template required_12216_02_mangos_disenchant_loot_template bit;

ALTER TABLE `disenchant_loot_template` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `disenchant_loot_template` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL DEFAULT '1';
