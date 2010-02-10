ALTER TABLE db_version CHANGE COLUMN required_8607_01_mangos_string required_8607_02_mangos_command bit;

DELETE FROM command where name='event activelist';

INSERT INTO `command` VALUES
('event list',2,'Syntax: .event list\r\nShow list of currently active events.\r\nShow list of all events');
