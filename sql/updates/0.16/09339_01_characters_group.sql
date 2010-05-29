ALTER TABLE character_db_version CHANGE COLUMN required_9250_01_characters_character required_9339_01_characters_group bit;

ALTER TABLE groups
  ADD COLUMN `groupId` int(11) unsigned NOT NULL FIRST;

UPDATE groups
  SET `groupId` = `leaderGuid`;

ALTER TABLE groups
  DROP PRIMARY KEY,
  ADD PRIMARY KEY (`groupId`),
  ADD UNIQUE KEY (`leaderGuid`);

ALTER TABLE group_member
  CHANGE COLUMN `leaderGuid` `groupId` int(11) unsigned NOT NULL;
