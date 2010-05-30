ALTER TABLE db_version CHANGE COLUMN required_9636_01_mangos_item_template required_9651_01_mangos_quest_poi bit;

-- Sorry, this was only way I knew, to avoid problems adding new primary key. Take backup if you don't want to loose your current data.
TRUNCATE quest_poi;
TRUNCATE quest_poi_points;

ALTER TABLE quest_poi ADD COLUMN poiId tinyint(3) UNSIGNED DEFAULT '0' NOT NULL AFTER questid;
ALTER TABLE quest_poi CHANGE COLUMN questid questId mediumint(8) UNSIGNED DEFAULT '0' NOT NULL;
ALTER TABLE quest_poi CHANGE COLUMN unk1 mapAreaId mediumint(8) UNSIGNED DEFAULT '0' NOT NULL;
ALTER TABLE quest_poi CHANGE COLUMN unk2 floorId tinyint(3) UNSIGNED DEFAULT '0' NOT NULL;

ALTER TABLE quest_poi_points ADD COLUMN poiId tinyint(3) UNSIGNED DEFAULT '0' NOT NULL AFTER questId;
ALTER TABLE quest_poi_points CHANGE COLUMN questId questId mediumint(8) UNSIGNED DEFAULT '0' NOT NULL;
ALTER TABLE quest_poi_points DROP COLUMN objIndex;

ALTER TABLE quest_poi DROP PRIMARY KEY,
  ADD PRIMARY KEY idx_poi (questId, poiId);

ALTER TABLE quest_poi_points DROP INDEX idx,
  ADD KEY idx_poip (questId, poiId);
