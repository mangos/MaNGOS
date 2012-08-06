ALTER TABLE db_version CHANGE COLUMN required_11117_01_mangos_world_template required_11169_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,393,394,395,396,397,398,399,1500,1501,1502);

INSERT INTO mangos_string VALUES
(376,'%u - |cffffffff|Hpool:%u|h[%s]|h|r AutoSpawn: %u MaxLimit: %u Creatures: %u GameObjecs: %u Pools %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(377,"No pools found for map '%s' (Id:%u)",NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(378,"You can't use this command at non-instanceable map '%s' (Id:%u). Use .lookup pool command instead.",NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(379,"You can't use this command without args at non-instanceable map '%s' (Id:%u).",NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(380,'%d%s - |cffffffff|Hcreature:%d|h[%s X:%f Y:%f Z:%f MapId:%d]|h|r Chance:%f %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(381,'%d%s - [%s] X:%f Y:%f Z:%f MapId:%d Chance:%f %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(382,'%d%s - |cffffffff|Hcreature:%d|h[%s X:%f Y:%f Z:%f MapId:%d]|h|r %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(383,'%d%s - [%s] X:%f Y:%f Z:%f MapId:%d %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(384,'%d%s - |cffffffff|Hgameobject:%d|h[%s X:%f Y:%f Z:%f MapId:%d]|h|r Chance:%f %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(385,'%d%s - [%s] X:%f Y:%f Z:%f MapId:%d Chance:%f %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(386,'%d%s - |cffffffff|Hgameobject:%d|h[%s X:%f Y:%f Z:%f MapId:%d]|h|r %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(387,'%d%s - [%s] X:%f Y:%f Z:%f MapId:%d %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(388,'Creatures with explicitly chance:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(389,'Creatures with equal chance:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(390,'Gameobjects with explicitly chance:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(391,'Gameobjects with equal chance:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(392,'Pool %u - Mother pool: %u |cffffffff|Hpool:%u|h[%s]|h|r AutoSpawn: %u MaxLimit: %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(393,'Pool %u - Mother pool: %u [%s] AutoSpawn: %u MaxLimit: %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(394,'Pool %u - Mother pool: none AutoSpawn: %u MaxLimit: %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(395,'No pools found',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(396,'%u - [%s] AutoSpawn: %u MaxLimit: %u Creatures: %u GameObjecs: %u Pools %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(397,'Pools with explicitly chance:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(398,'Pools with equal chance:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(399,'%u - |cffffffff|Hpool:%u|h[%s]|h|r AutoSpawn: %u MaxLimit: %u Creatures: %u GameObjecs: %u Pools %u Chance: %f %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1500,'%u - [%s] AutoSpawn: %u MaxLimit: %u Creatures: %u GameObjecs: %u Pools %u Chance: %f %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1501,'%u - |cffffffff|Hpool:%u|h[%s]|h|r AutoSpawn: %u MaxLimit: %u Creatures: %u GameObjecs: %u Pools %u %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1502,'%u - [%s] AutoSpawn: %u MaxLimit: %u Creatures: %u GameObjecs: %u Pools %u %s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
