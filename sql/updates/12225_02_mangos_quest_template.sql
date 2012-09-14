ALTER TABLE db_version CHANGE COLUMN required_12225_01_mangos_locales_quest required_12225_02_mangos_quest_template bit;

ALTER TABLE `quest_template` MODIFY COLUMN `PortraitGiverName` text AFTER `CompletedText`;
ALTER TABLE `quest_template` MODIFY COLUMN `PortraitGiverText` text AFTER `PortraitGiverName`;
ALTER TABLE `quest_template` MODIFY COLUMN `PortraitTurnInName` text AFTER `PortraitGiverText`;
ALTER TABLE `quest_template` MODIFY COLUMN `PortraitTurnInText` text AFTER `PortraitTurnInName`;
