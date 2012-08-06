ALTER TABLE db_version CHANGE COLUMN required_10237_01_mangos_spell_bonus_data required_10244_01_mangos_command bit;

DELETE FROM command WHERE name = 'stable';
INSERT INTO command (name, security, help) VALUES
('stable',3,'Syntax: .stable\r\n\r\nShow your pet stable.');
