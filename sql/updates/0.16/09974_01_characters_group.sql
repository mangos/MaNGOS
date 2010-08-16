ALTER TABLE character_db_version CHANGE COLUMN required_9849_01_characters_saved_variables required_9974_01_characters_group bit;

ALTER TABLE groups
  CHANGE COLUMN isRaid groupType tinyint(1) unsigned NOT NULL;

/* now fixed bug in past can save raids as 1 (BG group) */
UPDATE groups
  SET groupType = 2 WHERE groupType = 1;