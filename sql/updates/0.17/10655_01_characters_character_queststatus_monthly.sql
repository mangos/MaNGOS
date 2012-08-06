ALTER TABLE character_db_version CHANGE COLUMN required_10568_01_characters_character_tutorial required_10655_01_characters_character_queststatus_monthly bit;

DROP TABLE IF EXISTS character_queststatus_monthly;
CREATE TABLE character_queststatus_monthly (
  guid int(11) unsigned NOT NULL default '0' COMMENT 'Global Unique Identifier',
  quest int(11) unsigned NOT NULL default '0' COMMENT 'Quest Identifier',
  PRIMARY KEY  (guid,quest),
  KEY idx_guid (guid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Player System';

ALTER TABLE saved_variables
  ADD COLUMN NextMonthlyQuestResetTime bigint(40) unsigned NOT NULL default '0' AFTER NextWeeklyQuestResetTime;
