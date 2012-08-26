ALTER TABLE db_version CHANGE COLUMN required_10252_01_mangos_reputation_reward_rate required_10256_01_mangos_command bit;

DELETE FROM command WHERE name IN ('auction','auction aliance','auction goblin','auction horde');
INSERT INTO command (name, security, help) VALUES
('auction',3,'Syntax: .auction\r\n\r\nShow your team auction store.'),
('auction aliance',3,'Syntax: .auction aliance\r\n\r\nShow aliance auction store independent from your team.'),
('auction goblin',3,'Syntax: .auction goblin\r\n\r\nShow goblin auction store common for all teams.'),
('auction horde',3,'Syntax: .auction horde\r\n\r\nShow horde auction store independent from your team.');
