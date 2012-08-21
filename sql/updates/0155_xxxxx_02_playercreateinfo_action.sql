ALTER TABLE db_version CHANGE COLUMN required_0155_xxxxx_01_playercreateinfo_spell required_0155_xxxxx_02_playercreateinfo_action bit;

DELETE FROM `playercreateinfo_action` WHERE race IN (22);
INSERT INTO `playercreateinfo_action` VALUES
(22,1,0,88161,0),
(22,1,72,22627,1),
(22,1,73,22625,1),
(22,3,0,3044,0),
(22,3,10,9,0),
(22,3,11,982,0),
(22,4,0,1752,0),
(22,5,0,585,0),
(22,6,0,6603,0),
(22,6,1,49576,0),
(22,6,2,45477,0),
(22,6,3,45462,0),
(22,6,4,45902,0),
(22,6,5,47541,0),
(22,6,9,3456,1),
(22,8,0,133,0),
(22,9,0,686,0),
(22,9,10,10,0),
(22,11,0,5176,0);
