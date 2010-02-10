ALTER TABLE db_version CHANGE COLUMN required_9074_01_mangos_command required_9095_01_mangos_command bit;

DELETE FROM command where name IN ('modify scale');

INSERT INTO `command` VALUES
('modify scale',1,'Syntax: .modify scale #scale\r\n\r\nChange model scale for targeted player (util relogin) or creature (until respawn).');
