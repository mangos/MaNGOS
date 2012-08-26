ALTER TABLE db_version CHANGE COLUMN required_12112_01_mangos_item_template required_12112_02_mangos_quest_template bit;

ALTER TABLE `quest_template` MODIFY COLUMN `RequiredRaces` int(7) unsigned NOT NULL DEFAULT '0';

ALTER TABLE `quest_template` ADD COLUMN `ReqSpellLearned` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `ReqSpellCast4`;

ALTER TABLE `quest_template` ADD COLUMN `PortraitGiver` mediumint(9) NOT NULL DEFAULT '0' AFTER `BonusTalents`;
ALTER TABLE `quest_template` ADD COLUMN `PortraitTurnIn` mediumint(9) NOT NULL DEFAULT '0' AFTER `PortraitGiver`;

ALTER TABLE `quest_template` ADD COLUMN `PortraitGiverText` text AFTER `CompletedText`;
ALTER TABLE `quest_template` ADD COLUMN `PortraitGiverName` text AFTER `PortraitGiverText`;
ALTER TABLE `quest_template` ADD COLUMN `PortraitTurnInText` text AFTER `PortraitGiverName`;
ALTER TABLE `quest_template` ADD COLUMN `PortraitTurnInName` text AFTER `PortraitTurnInText`;

ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyId1` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `ReqCreatureOrGOCount4`;
ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyId2` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `ReqCurrencyId1`;
ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyId3` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `ReqCurrencyId2`;
ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyId4` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `ReqCurrencyId3`;
ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyCount1` mediumint(9) NOT NULL DEFAULT '0' AFTER `ReqCurrencyId4`;
ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyCount2` mediumint(9) NOT NULL DEFAULT '0' AFTER `ReqCurrencyCount1`;
ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyCount3` mediumint(9) NOT NULL DEFAULT '0' AFTER `ReqCurrencyCount2`;
ALTER TABLE `quest_template` ADD COLUMN `ReqCurrencyCount4` mediumint(9) NOT NULL DEFAULT '0' AFTER `ReqCurrencyCount3`;

ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyId1` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `RewItemCount4`;
ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyId2` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `RewCurrencyId1`;
ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyId3` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `RewCurrencyId2`;
ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyId4` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `RewCurrencyId3`;
ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyCount1` mediumint(9) NOT NULL DEFAULT '0' AFTER `RewCurrencyId4`;
ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyCount2` mediumint(9) NOT NULL DEFAULT '0' AFTER `RewCurrencyCount1`;
ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyCount3` mediumint(9) NOT NULL DEFAULT '0' AFTER `RewCurrencyCount2`;
ALTER TABLE `quest_template` ADD COLUMN `RewCurrencyCount4` mediumint(9) NOT NULL DEFAULT '0' AFTER `RewCurrencyCount3`;

ALTER TABLE `quest_template` ADD COLUMN `RewSkill` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `RewCurrencyCount4`;
ALTER TABLE `quest_template` ADD COLUMN `RewSkillValue` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `RewSkill`;

ALTER TABLE `quest_template` ADD COLUMN `SoundAccept` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `OfferRewardEmoteDelay4`;
ALTER TABLE `quest_template` ADD COLUMN `SoundTurnIn` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `SoundAccept`;
