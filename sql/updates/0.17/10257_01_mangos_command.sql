ALTER TABLE db_version CHANGE COLUMN required_10256_01_mangos_command required_10257_01_mangos_command bit;

DELETE FROM command WHERE name IN ('auction aliance','auction alliance');
INSERT INTO command (name, security, help) VALUES
('auction alliance',3,'Syntax: .auction alliance\r\n\r\nShow alliance auction store independent from your team.');
