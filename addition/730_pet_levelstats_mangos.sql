-- pets rewrite
ALTER TABLE `pet_levelstats` ADD `mindmg` MEDIUMINT( 11 ) NOT NULL DEFAULT '0' COMMENT 'Min base damage' AFTER `armor` ,
ADD `maxdmg` MEDIUMINT( 11 ) NOT NULL DEFAULT '0' COMMENT 'Max base damage' AFTER `mindmg`,
ADD `attackpower` MEDIUMINT( 11 ) NOT NULL DEFAULT '0' COMMENT 'Attack power' AFTER `maxdmg`;
