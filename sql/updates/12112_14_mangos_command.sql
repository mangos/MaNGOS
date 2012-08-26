ALTER TABLE db_version CHANGE COLUMN required_12112_13_mangos_player_levelstats required_12112_14_mangos_command bit;

DELETE FROM `command` WHERE `name` IN ('npc addcurrency', 'npc delcurrency');
INSERT INTO `command` VALUES
('npc addcurrency',2,'Syntax: .npc addcurrency #currencyId #buycount #extendedcost\r\n\r\nAdd currency #currencyId to item list of selected vendor.'),
('npc delcurrency',2,'Syntax: .npc delcurrency #currencyId\r\n\r\nRemove currency #currencyId from item list of selected vendor.');
