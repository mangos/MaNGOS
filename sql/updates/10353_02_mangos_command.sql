ALTER TABLE db_version CHANGE COLUMN required_10353_01_mangos_mangos_string required_10353_02_mangos_command bit;

DELETE FROM command WHERE name IN ('ticket');
INSERT INTO command (name, security, help) VALUES
('ticket',2,'Syntax: .ticket on\r\n        .ticket off\r\n        .ticket #num\r\n        .ticket $character_name\r\n        .ticket respond #num $response\r\n        .ticket respond $character_name $response\r\n\r\non/off for GMs to show or not a new ticket directly, $character_name to show ticket of this character, #num to show ticket #num.');
