ALTER TABLE db_version CHANGE COLUMN required_12195_02_mangos_mangos_string required_12216_01_mangos_creature_loot_template bit;

ALTER TABLE `creature_loot_template` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `creature_loot_template` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL DEFAULT '1';
