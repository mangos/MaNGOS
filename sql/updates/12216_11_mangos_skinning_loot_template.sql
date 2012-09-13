ALTER TABLE db_version CHANGE COLUMN required_12216_10_mangos_reference_loot_template required_12216_11_mangos_skinning_loot_template bit;

ALTER TABLE `skinning_loot_template` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `skinning_loot_template` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL DEFAULT '1';
