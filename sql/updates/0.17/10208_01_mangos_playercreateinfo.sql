ALTER TABLE db_version CHANGE COLUMN required_10207_01_mangos_npc_vendor required_10208_01_mangos_playercreateinfo bit;

UPDATE playercreateinfo SET orientation= 5.696318 WHERE race=4 and class<>6;
UPDATE playercreateinfo SET orientation= 6.177156 WHERE race=3 and class<>6;
UPDATE playercreateinfo SET orientation= 2.70526 WHERE race=5 and class<>6;
UPDATE playercreateinfo SET orientation= 5.316046 WHERE race=10 and class<>6;
UPDATE playercreateinfo SET orientation= 2.083644 WHERE race=11 and class<>6;
UPDATE playercreateinfo SET orientation= 3.659973 WHERE class=6;
