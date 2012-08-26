ALTER TABLE db_version CHANGE COLUMN required_11731_01_mangos_command required_11731_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (1171,1172,1173,1174,1175,1176,1177,1178,1179,1180,1181,1182,1183,1184,1185,1186,1187,1188,1189,1190,1191);

INSERT INTO mangos_string VALUES
(1171,'All config are reloaded from ahbot configuration file.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1172,'Error while trying to reload ahbot config.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1173,'==========================================================',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1174,'|--------------------------------------------------------|',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1175,'|            | Alliance |  Horde   | Neutral  |  Total   |',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1176,'          Alliance/Horde/Neutral/Total',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1177,'| %-10s | %8u | %8u | %8u | %8u |',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1178,'%-10s = %6u / %6u / %6u / %6u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1179,'Count',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1180,'Item Ratio',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1181,'|            | Alliance |   Horde  | Neutral  |  Amount  |',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1182,'          Alliance/Horde/Neutral/Amount',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1183,'Grey',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1184,'White',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1185,'Green',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1186,'Blue',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1187,'Purple',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1188,'Orange',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1189,'Yellow',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1190,'Amount of %s items is set to %u.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1191,'Items ratio for %s is set to %u.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
