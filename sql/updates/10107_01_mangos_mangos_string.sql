ALTER TABLE db_version CHANGE COLUMN required_10106_02_mangos_mangos_string required_10107_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry BETWEEN 1143 AND 1148;

INSERT INTO mangos_string VALUES
(1143, 'Spawned by event %u (%s)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1144, 'Despawned by event %u (%s)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1145, 'Part of pool %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1146, 'Part of pool %u, top pool %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1147, 'The (top)pool %u is spawned by event %u (%s)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1148, 'The (top)pool %u is despawned by event %u (%s)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
