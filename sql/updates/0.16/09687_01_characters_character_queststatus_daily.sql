ALTER TABLE character_db_version CHANGE COLUMN required_9686_01_characters_character_queststatus_weekly required_9687_01_characters_character_queststatus_daily bit;

ALTER TABLE `character_queststatus_daily`
  DROP COLUMN `time`;

ALTER TABLE `saved_variables`
  ADD COLUMN `NextDailyQuestResetTime` bigint(40) unsigned NOT NULL default '0' AFTER `NextArenaPointDistributionTime`;
