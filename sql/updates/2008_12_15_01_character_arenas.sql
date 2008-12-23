ALTER TABLE character_db_version CHANGE COLUMN required_2008_12_03_01_character_guild_member required_2008_12_15_01_character_arenas bit;

CREATE TABLE `saved_variables` (
    `NextArenaPointDistributionTime` bigint(40) UNSIGNED NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Variable Saves';

ALTER TABLE `arena_team_member` ADD COLUMN `personal_rating` int(10) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `characters` ADD COLUMN `arena_pending_points` int(10) UNSIGNED NOT NULL default '0';
