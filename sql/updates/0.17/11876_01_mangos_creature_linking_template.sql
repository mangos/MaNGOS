ALTER TABLE db_version CHANGE COLUMN required_11852_01_mangos_gossip_menu required_11876_01_mangos_creature_linking_template bit;

ALTER TABLE `creature_linking_template` CHANGE COLUMN `entry` `entry` mediumint(8) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'creature_template.entry of the slave mob that is linked';
ALTER TABLE `creature_linking_template` CHANGE COLUMN `map` `map` SMALLINT(5) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Id of map of the mobs';
ALTER TABLE `creature_linking_template` CHANGE COLUMN `master_entry` `master_entry` mediumint(8) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'master to trigger events';
ALTER TABLE `creature_linking_template` CHANGE COLUMN `flag` `flag` mediumint(8) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'flag - describing what should happen when';
