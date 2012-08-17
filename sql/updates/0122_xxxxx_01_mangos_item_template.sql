ALTER TABLE db_version CHANGE COLUMN required_0114_xxxxx_01_mangos_item_template required_0122_xxxxx_01_mangos_item_template bit;

ALTER TABLE `item_template` DROP `DamageType`;

ALTER TABLE `item_template` ADD COLUMN `DamageType` tinyint(3) unsigned NOT NULL DEFAULT '0' AFTER `ScalingStatDistribution`;
