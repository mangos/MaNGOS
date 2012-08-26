ALTER TABLE db_version CHANGE COLUMN required_11831_01_mangos_mangos_string required_11831_02_mangos_command bit;

DELETE FROM command WHERE name = 'gearscore';
INSERT INTO command (name, security, help) VALUES
('gearscore',3,'Syntax: .gearscore [#withBags] [#withBank]\r\n\r\nShow selected player\'s gear score. Check items in bags if #withBags != 0 and check items in Bank if #withBank != 0. Default: 1 for bags and 0 for bank');