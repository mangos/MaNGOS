ALTER TABLE character_db_version CHANGE COLUMN required_9767_03_characters_characters required_9849_01_characters_saved_variables bit;

ALTER TABLE saved_variables ADD cleaning_flags int(11) unsigned NOT NULL default '0' AFTER NextWeeklyQuestResetTime;
UPDATE saved_variables SET cleaning_flags = 0xF;
