ALTER TABLE db_version CHANGE COLUMN required_10217_01_mangos_playercreateinfo_spell required_10217_02_mangos_playercreateinfo_action bit;


DELETE FROM playercreateinfo_action WHERE action  IN (117, 21084, 159, 2070, 4540);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (1, 1, 1, 78, 0),
 (1, 1, 9, 59752, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (1, 2, 1, 20154, 0);

DELETE FROM playercreateinfo_action WHERE class=5 AND race=1 AND button IN (0,1,2);
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (1, 5, 0, 585, 0),
 (1, 5, 1, 2050, 0);

DELETE FROM playercreateinfo_action WHERE class=8 AND race=1 AND button IN (0,1,2);
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (1, 8, 0, 133, 0),
 (1, 8, 1, 168, 0);

DELETE FROM playercreateinfo_action WHERE class=9 AND race=1 AND button IN (0,1,2);
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (1, 9, 0, 686, 0),
 (1, 9, 1, 687, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (2, 1, 1, 78, 0),
 (2, 1, 2, 20572, 0);

DELETE FROM playercreateinfo_action where race=2 AND class=9;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (2, 9, 0, 686, 0),
 (2, 9, 1, 687, 0),
 (2, 9, 2, 33702, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (3, 1, 2, 20594, 0),
 (3, 1, 3, 2481, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (3, 2, 1, 20154, 0);

DELETE FROM playercreateinfo_action WHERE class=5 AND race=3 AND button IN (0,1,2,3,4);
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (3, 5, 0, 585, 0),
 (3, 5, 1, 2050, 0),
 (3, 5, 2, 20594, 0),
 (3, 5, 3, 2481, 0);

DELETE FROM playercreateinfo_action WHERE class=1 AND race=4;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (4, 1, 0, 6603, 0),
 (4, 1, 1, 78, 0),
 (4, 1, 108, 6603, 0),
 (4, 1, 2, 58984, 0),
 (4, 1, 72, 6603, 0),
 (4, 1, 73, 78, 0),
 (4, 1, 74, 58984, 0),
 (4, 1, 84, 6603, 0),
 (4, 1, 96, 6603, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (4, 3, 81, 58984, 0);

DELETE FROM playercreateinfo_action WHERE race=4 AND class=4 AND button=10;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (4, 4, 4, 58984, 0);

DELETE FROM playercreateinfo_action WHERE race=4 AND class=5 ;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (4, 5, 0, 585, 0),
 (4, 5, 1, 2050, 0),
 (4, 5, 2, 58984, 0),
 (4, 5, 81, 58984, 0);

DELETE FROM playercreateinfo_action WHERE race=4 AND class=11 ;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (4, 11, 0, 5176, 0),
 (4, 11, 1, 5185, 0),
 (4, 11, 108, 6603, 0),
 (4, 11, 2, 58984, 0),
 (4, 11, 72, 6603, 0),
 (4, 11, 74, 58984, 0),
 (4, 11, 84, 6603, 0),
 (4, 11, 96, 6603, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (5, 1, 1, 78, 0),
 (5, 1, 2, 20577, 0);

DELETE FROM playercreateinfo_action where race=5 AND class=8;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (5, 8, 0, 133, 0),
 (5, 8, 1, 168, 0),
 (5, 8, 2, 20577, 0);

DELETE FROM playercreateinfo_action where race=5 AND class=9;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (5, 9, 0, 686, 0),
 (5, 9, 1, 687, 0),
 (5, 9, 2, 20577, 0);

DELETE FROM playercreateinfo_action where race=5 AND class=5;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (5, 5, 0, 585, 0),
 (5, 5, 1, 2050, 0),
 (5, 5, 2, 20577, 0);

UPDATE playercreateinfo_action SET button=2 WHERE race=6 AND class=1 AND button=3;
UPDATE playercreateinfo_action SET button=75 WHERE race=6 AND class=3 AND button=76;
UPDATE playercreateinfo_action SET button=75 WHERE race=6 AND class=7 AND button=76;

DELETE FROM playercreateinfo_action WHERE race=6 AND class=11;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (6, 11, 0, 5176, 0),
 (6, 11, 1, 5185, 0),
 (6, 11, 108, 6603, 0),
 (6, 11, 2, 20549, 0),
 (6, 11, 72, 6603, 0),
 (6, 11, 75, 20549, 0),
 (6, 11, 84, 6603, 0),
 (6, 11, 96, 6603, 0);

DELETE FROM playercreateinfo_action WHERE race=7 AND class=1 AND button in (82,10);
DELETE FROM playercreateinfo_action WHERE race=7 AND class=4 AND button in (10);
DELETE FROM playercreateinfo_action WHERE race=7 AND class=8;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (7, 8, 0, 133, 0),
 (7, 8, 1, 168, 0);

DELETE FROM playercreateinfo_action WHERE race=7 AND class=9;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (7, 9, 0, 686, 0),
 (7, 9, 1, 687, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (8, 1, 1, 78, 0),
 (8, 1, 2, 26297, 0);

INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (8, 4, 76, 26297, 0);

DELETE FROM playercreateinfo_action WHERE race=8 AND class=5;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (8, 5, 0, 585, 0),
 (8, 5, 1, 2050, 0),
 (8, 5, 2, 26297, 0);

DELETE FROM playercreateinfo_action WHERE race=8 AND class=8;
INSERT IGNORE INTO playercreateinfo_action (race, class, button, action, type) VALUES
 (8, 8, 0, 133, 0),
 (8, 8, 1, 168, 0),
 (8, 8, 2, 26297, 0);

