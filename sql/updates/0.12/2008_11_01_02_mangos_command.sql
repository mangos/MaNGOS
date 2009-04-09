ALTER TABLE db_version CHANGE COLUMN required_2008_11_01_01_mangos_mangos_string required_2008_11_01_02_mangos_command bit;

DELETE FROM command WHERE name IN ('modify gender');

INSERT INTO command VALUES
('modify gender',2,'Syntax: .modify gender male/female\r\n\r\nChange gender of selected player.');
