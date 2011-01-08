-- Refer-a-friend system by MasOn
ALTER TABLE `characters` ADD COLUMN `grantableLevels`  tinyint(3) unsigned NOT NULL default '0' AFTER `actionBars`;