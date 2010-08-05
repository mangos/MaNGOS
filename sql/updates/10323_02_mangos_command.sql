ALTER TABLE db_version CHANGE COLUMN required_10323_01_mangos_mangos_string required_10323_02_mangos_command bit;

DELETE FROM command WHERE name IN ('lookup achievement','character achievements');
INSERT INTO command (name, security, help) VALUES
('character achievements',2,'Syntax: .character achievements [$player_name]\r\n\r\nShow completed achievments for selected player or player find by $player_name.'),
('lookup achievement',2,'Syntax: .lookup $name\r\nLooks up a achievement by $namepart, and returns all matches with their quest ID\'s. Achievement shift-links generated with information about achievment state for selected player. Also for completed achievments in list show complete date.');
