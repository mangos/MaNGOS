ALTER TABLE db_version CHANGE COLUMN required_11521_01_mangos_command required_11523_01_mangos_command bit;

DELETE FROM command WHERE name = 'npc aiinfo';

INSERT INTO command (name, security, help) VALUES
('npc aiinfo',2,'Syntax: .npc npc aiinfo\r\n\r\nShow npc AI and script information.');
