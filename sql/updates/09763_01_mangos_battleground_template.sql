ALTER TABLE db_version CHANGE COLUMN required_9761_01_mangos_mangos_string required_9763_01_mangos_battleground_template bit;

ALTER TABLE battleground_template
  DROP COLUMN MinLvl,
  DROP COLUMN MaxLvl;

DELETE FROM `battleground_template`;
INSERT INTO `battleground_template` VALUES
(1,40,40,611,2.72532,610,2.27452),
(2,10,10,769,3.14159,770,3.14159),
(3,15,15,890,3.40156,889,0.263892),
(4,5,5,929,0,936,3.14159),
(5,5,5,939,0,940,3.14159),
(6,5,5,0,0,0,0),
(7,15,15,1103,3.40156,1104,0.263892),
(8,5,5,1258,0,1259,3.14159),
(9,15,15,1367,0,1368,0),
(10,5,5,1362,0,1363,0),
(11,5,5,1364,0,1365,0),
(30,40,40,1485,0,1486,0),
(32,5,40,0,0,0,0);
