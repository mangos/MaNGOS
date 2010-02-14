ALTER TABLE db_version CHANGE COLUMN required_9379_01_mangos_spell_proc_event required_9380_01_mangos_command bit;

DELETE FROM command WHERE name IN ('modify fly', 'npc say', 'npc yell');
INSERT INTO command VALUES
('modify fly', 1, 'Syntax: .modify fly #rate\r\n.fly #rate\r\n\r\nModify the flying speed of the selected player to \"normal base fly speed\"*rate. If no player is selected, modify your fly.\r\n\r\n #rate may range from 0.1 to 10.'),
('npc say', 1, 'Syntax: .npc say #text\r\nMake the selected npc says #text.'),
('npc yell', 1, 'Syntax: .npc yell #text\r\nMake the selected npc yells #text.');
