ALTER TABLE db_version CHANGE COLUMN required_11169_01_mangos_mangos_string required_11169_02_mangos_command bit;

DELETE FROM command WHERE name IN ('lookup pool','pool list','pool spawns','pool');

INSERT INTO command (name, security, help) VALUES
('lookup pool',2,'Syntax: .lookup pool $pooldescpart\r\n\r\nList of pools (anywhere) with substring in description.'),
('pool list',2,'Syntax: .pool list\r\n\r\nList of pools with spawn in current map (only work in instances. Non-instanceable maps share pool system state os useless attempt get all pols at all continents.'),
('pool spawns',2,'Syntax: .pool spawns #pool_id\r\n\r\nList current creatures/objects listed in pools (or in specific #pool_id) and spawned (added to grid data, not meaning show in world.'),
('pool',2,'Syntax: .pool #pool_id\r\n\r\nPool information and full list creatures/gameobjects included in pool.');
