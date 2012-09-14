ALTER TABLE db_version CHANGE COLUMN required_12216_12_mangos_spell_loot_template required_12225_01_mangos_locales_quest bit;

ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc1` text AFTER `ObjectiveText4_loc8`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc2` text AFTER `PortraitGiverName_loc1`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc3` text AFTER `PortraitGiverName_loc2`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc4` text AFTER `PortraitGiverName_loc3`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc5` text AFTER `PortraitGiverName_loc4`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc6` text AFTER `PortraitGiverName_loc5`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc7` text AFTER `PortraitGiverName_loc6`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverName_loc8` text AFTER `PortraitGiverName_loc7`;

ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc1` text AFTER `PortraitGiverName_loc8`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc2` text AFTER `PortraitGiverText_loc1`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc3` text AFTER `PortraitGiverText_loc2`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc4` text AFTER `PortraitGiverText_loc3`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc5` text AFTER `PortraitGiverText_loc4`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc6` text AFTER `PortraitGiverText_loc5`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc7` text AFTER `PortraitGiverText_loc6`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitGiverText_loc8` text AFTER `PortraitGiverText_loc7`;

ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc1` text AFTER `PortraitGiverText_loc8`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc2` text AFTER `PortraitTurnInName_loc1`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc3` text AFTER `PortraitTurnInName_loc2`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc4` text AFTER `PortraitTurnInName_loc3`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc5` text AFTER `PortraitTurnInName_loc4`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc6` text AFTER `PortraitTurnInName_loc5`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc7` text AFTER `PortraitTurnInName_loc6`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInName_loc8` text AFTER `PortraitTurnInName_loc7`;

ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc1` text AFTER `PortraitTurnInName_loc8`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc2` text AFTER `PortraitTurnInText_loc1`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc3` text AFTER `PortraitTurnInText_loc2`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc4` text AFTER `PortraitTurnInText_loc3`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc5` text AFTER `PortraitTurnInText_loc4`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc6` text AFTER `PortraitTurnInText_loc5`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc7` text AFTER `PortraitTurnInText_loc6`;
ALTER TABLE `locales_quest` ADD COLUMN `PortraitTurnInText_loc8` text AFTER `PortraitTurnInText_loc7`;
