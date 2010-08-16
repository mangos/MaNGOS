ALTER TABLE db_version CHANGE COLUMN required_10089_01_mangos_game_event_pool required_10106_01_mangos_command bit;

DELETE FROM command WHERE name IN('go', 'go creature','go object');
INSERT INTO command (name, security, help) VALUES
('go',1,'Syntax: .go  [$playername|pointlink|#x #y #z [#mapid]]\r\nTeleport your character to point with coordinates of player $playername, or coordinates of one from shift-link types: player, tele, taxinode, creature/creature_entry, gameobject/gameobject_entry, or explicit #x #y #z #mapid coordinates.'),
('go creature',1,'Syntax: .go creature (#creature_guid|$creature_name|id #creature_id)\r\nTeleport your character to creature with guid #creature_guid, or teleport your character to creature with name including as part $creature_name substring, or teleport your character to a creature that was spawned from the template with this entry #creature_id.'),
('go object',1,'Syntax: .go object (#gameobject_guid|$gameobject_name|id #gameobject_id)\r\nTeleport your character to gameobject with guid #gameobject_guid, or teleport your character to gameobject with name including as part $gameobject_name substring, or teleport your character to a gameobject that was spawned from the template with this entry #gameobject_id.');
