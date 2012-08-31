ALTER TABLE db_version CHANGE COLUMN required_12150_01_mangos_mangos_string required_12164_01_mangos_command  bit;


DELETE FROM `command` WHERE `name` = 'honor updatekills';
INSERT INTO `command` VALUE
('honor updatekills',2,'Syntax: .honor updatekills\r\n\r\nForce the yesterday\'s honor kill fields to be updated with today\'s data, which will get reset for the selected player.');
