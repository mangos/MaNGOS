ALTER TABLE db_version CHANGE COLUMN required_12216_03_mangos_fishing_loot_template required_12216_04_mangos_gameobject_loot_template bit;

ALTER TABLE `gameobject_loot_template` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `gameobject_loot_template` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL DEFAULT '1';
