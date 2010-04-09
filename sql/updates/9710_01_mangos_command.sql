ALTER TABLE db_version CHANGE COLUMN required_9704_01_mangos_achievement_reward required_9710_01_mangos_command bit;

DELETE FROM command WHERE name IN ('reset specs','reset talents');
INSERT INTO command VALUES
('reset specs',3,'Syntax: .reset specs [Playername]\r\n  Removes all talents (for all specs) of the targeted player or named player. Playername can be name of offline character. With player talents also will be reset talents for all character\'s pets if any.'),
('reset talents',3,'Syntax: .reset talents [Playername]\r\n  Removes all talents (current spec) of the targeted player or pet or named player. With player talents also will be reset talents for all character\'s pets if any.');
