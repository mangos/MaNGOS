ALTER TABLE db_version CHANGE COLUMN required_2008_11_14_01_mangos_scripts required_2008_11_16_01_mangos_command bit;

DELETE FROM `command` WHERE `name` IN (
  'server exit',
  'server idleshutdown',
  'server idleshutdown cancel',
  'server idlerestart',
  'server idlerestart cancel',
  'server restart',
  'server restart cancel',
  'server shutdown',
  'server shutdown cancel'
);

INSERT INTO `command` (`name`, `security`, `help`) VALUES
('server exit',4,'Syntax: .server exit\r\n\r\nTerminate mangosd NOW. Exit code 0.'),
('server idleshutdown',3,'Syntax: .server idleshutdown #delay [#exist_code]\r\n\r\nShut the server down after #delay seconds if no active connections are present (no players). Use #exist_code or 0 as program exist code.'),
('server idleshutdown cancel',3,'Syntax: .server idleshutdown cancel\r\n\r\nCancel the restart/shutdown timer if any.'),
('server idlerestart',3,'Syntax: .server idlerestart #delay\r\n\r\nRestart the server after #delay seconds if no active connections are present (no players). Use #exist_code or 2 as program exist code.'),
('server idlerestart cancel',3,'Syntax: .server idlerestart cancel\r\n\r\nCancel the restart/shutdown timer if any.'),
('server restart',3,'Syntax: .server restart #delay\r\n\r\nRestart the server after #delay seconds. Use #exist_code or 2 as program exist code.'),
('server restart cancel',3,'Syntax: .server restart cancel\r\n\r\nCancel the restart/shutdown timer if any.'),
('server shutdown',3,'Syntax: .server shutdown #delay [#exist_code]\r\n\r\nShut the server down after #delay seconds. Use #exist_code or 0 as program exist code.'),
('server shutdown cancel',3,'Syntax: .server shutdown cancel\r\n\r\nCancel the restart/shutdown timer if any.');
