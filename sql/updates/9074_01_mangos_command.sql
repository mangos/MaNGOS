ALTER TABLE db_version CHANGE COLUMN required_9070_01_mangos_spell_proc_event required_9074_01_mangos_command bit;

DELETE FROM command where name IN ('server shutdown');

INSERT INTO `command` VALUES
('server shutdown',3,'Syntax: .server shutdown #delay [#exit_code]\r\n\r\nShut the server down after #delay seconds. Use #exit_code or 0 as program exit code.');
