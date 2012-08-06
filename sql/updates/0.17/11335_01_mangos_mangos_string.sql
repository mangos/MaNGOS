ALTER TABLE db_version CHANGE COLUMN required_11312_01_mangos_mangos_string required_11335_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1170);

INSERT INTO mangos_string VALUES
(1170,'Player selected NPC\nGUID: %u.\nFaction: %u.\nnpcFlags: %u.\nBase Entry: %u, Spawned Entry %u (Difficulty %u).\nDisplayID: %u (Native: %u).',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
