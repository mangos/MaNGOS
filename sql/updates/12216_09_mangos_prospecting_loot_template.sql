ALTER TABLE db_version CHANGE COLUMN required_12216_08_mangos_pickpocketing_loot_template required_12216_09_mangos_prospecting_loot_template bit;

ALTER TABLE `prospecting_loot_template` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `prospecting_loot_template` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL DEFAULT '1';
