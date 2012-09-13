ALTER TABLE db_version CHANGE COLUMN required_12216_06_mangos_mail_loot_template required_12216_07_mangos_milling_loot_template bit;

ALTER TABLE `milling_loot_template` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `milling_loot_template` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL DEFAULT '1';
