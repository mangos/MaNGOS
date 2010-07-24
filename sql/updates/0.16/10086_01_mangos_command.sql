ALTER TABLE db_version CHANGE COLUMN required_10056_01_mangos_spell_proc_event required_10086_01_mangos_command bit;

DELETE FROM command WHERE name IN('go');
INSERT INTO command (name, security, help) VALUES
('go',1,'Syntax: .go  [$playername|pointlink|#x #y #z [#mapid]]\r\nTeleport your character to point with coordinates of player $playername, or coordinates of one from shift-link types: player, tele, taxinode, creature, gameobject, or explicit #x #y #z #mapid coordinates.');
