ALTER TABLE `saved_variables` ADD COLUMN `NextRandomBGResetTime` bigint(40) unsigned NOT NULL default '0' AFTER `NextWeeklyQuestResetTime`;
