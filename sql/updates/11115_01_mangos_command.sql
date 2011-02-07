ALTER TABLE db_version CHANGE COLUMN required_11058_01_mangos_spell_proc_event required_11115_01_mangos_command bit;

DELETE FROM command WHERE name = 'gm setview';

INSERT INTO command (name, security, help) VALUES
('gm setview',1,'Syntax: .gm setview\r\n\r\nSet farsight view on selected unit. Select yourself to set view back.');

