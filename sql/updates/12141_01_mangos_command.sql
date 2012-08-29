ALTER TABLE db_version CHANGE COLUMN required_12138_01_mangos_command required_12141_01_mangos_command  bit;

DELETE FROM `command` WHERE `name` IN ('modify honor', 'modify arena', 'modify currency', 'lookup currency');
INSERT INTO `command` VALUES
('modify currency',2,'Syntax: .modify currency $id $amount\r\n\r\nAdd $amount points of currency $id to the selected player.'),
('lookup currency',3,'Syntax: .lookup currency $namepart\r\n\r\nLooks up a currency by $namepart, and returns all matches.');
