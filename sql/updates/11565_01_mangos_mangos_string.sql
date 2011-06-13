ALTER TABLE db_version CHANGE COLUMN required_11560_01_mangos_command required_11565_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1170,539);

INSERT INTO mangos_string VALUES
(539,'Player selected: %s.\nFaction: %u.\nnpcFlags: %u.\nEntry: %u.\nDisplayID: %u (Native: %u).',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1170,'Player selected: %s.\nFaction: %u.\nnpcFlags: %u.\nBase Entry: %u, Spawned Entry %u (Difficulty %u).\nDisplayID: %u (Native: %u).',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
