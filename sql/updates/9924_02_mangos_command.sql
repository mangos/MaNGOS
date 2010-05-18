ALTER TABLE db_version CHANGE COLUMN required_9924_01_mangos_mangos_string required_9924_02_mangos_command bit;

DELETE FROM command WHERE name IN('server set loglevel','server log level','server log filter');
INSERT INTO command (name, security, help) VALUES
('server log filter',4,'Syntax: .server log filter [($filtername|all) (on|off)]\r\n\r\nShow or set server log filters. If used "all" then all filters will be set to on/off state.'),
('server log level',4,'Syntax: .server log level [#level]\r\n\r\nShow or set server log level (0 - errors only, 1 - basic, 2 - detail, 3 - debug).');
