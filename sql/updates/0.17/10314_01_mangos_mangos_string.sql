ALTER TABLE db_version CHANGE COLUMN required_10307_03_mangos_scripted_event_id required_10314_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,512,1105,1152);

INSERT INTO mangos_string VALUES
(357,'Areatrigger %u not has target coordinates',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(358,'No areatriggers found!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(359,'%s|cffffffff|Hareatrigger_target:%u|h[Trigger target %u]|h|r Map %u X:%f Y:%f Z:%f%s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(360,'%s[Trigger target %u] Map %u X:%f Y:%f Z:%f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(361,'|cffffffff|Hareatrigger:%u|h[Trigger %u]|h|r Map %u X:%f Y:%f Z:%f%s%s%s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(362,'[Trigger %u] Map %u X:%f Y:%f Z:%f%s%s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(363,' (Dist %f)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(364,' [Tavern]',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(365,' [Quest]',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(366,'Explore quest:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(367,'Required level %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(368,'Required Items:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(369,'Required quest (normal difficulty):',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(370,'Required heroic keys:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(371,'Required quest (heroic difficulty):',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(512,'%d - |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1105,'%d - %s %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1152,'[usable]',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
