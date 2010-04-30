ALTER TABLE character_db_version CHANGE COLUMN required_9611_01_characters required_9630_01_characters_characters bit;

ALTER TABLE characters
  ADD COLUMN `exploredZones` longtext AFTER activeSpec,
  ADD COLUMN `equipmentCache` longtext AFTER exploredZones,
  ADD COLUMN `ammoId` int(10) UNSIGNED NOT NULL default '0' AFTER equipmentCache;

UPDATE characters SET
exploredZones = SUBSTRING(data,
length(SUBSTRING_INDEX(data, ' ', 1041))+2,
length(SUBSTRING_INDEX(data, ' ', 1168+1))- length(SUBSTRING_INDEX(data, ' ', 1041)) - 1),
equipmentCache = SUBSTRING(data,
length(SUBSTRING_INDEX(data, ' ', 283))+2,
length(SUBSTRING_INDEX(data, ' ', 320+1))- length(SUBSTRING_INDEX(data, ' ', 283)) - 1),
ammoId = SUBSTRING(data,
length(SUBSTRING_INDEX(data, ' ', 1198))+2,
length(SUBSTRING_INDEX(data, ' ', 1198+1))- length(SUBSTRING_INDEX(data, ' ', 1198)) - 1);

CREATE TABLE `data_backup` (
  `guid` int(11) unsigned NOT NULL default '0' COMMENT 'Global Unique Identifier',
  `data` longtext,
  PRIMARY KEY  (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT INTO data_backup (guid, data)  (SELECT guid, data FROM characters);


ALTER TABLE characters
  DROP COLUMN data;
