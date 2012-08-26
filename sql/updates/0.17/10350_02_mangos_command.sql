ALTER TABLE db_version CHANGE COLUMN required_10349_01_mangos_spell_proc_event required_10350_02_mangos_command bit;

DELETE FROM command WHERE name IN ('trigger active','go trigger');
INSERT INTO command (name, security, help) VALUES
('go trigger',1,'Syntax: .go trigger (#trigger_id|$trigger_shift-link|$trigger_target_shift-link) [target]\r\n\r\nTeleport your character to areatrigger with id #trigger_id or trigger id associated with shift-link. If additional arg "target" provided then character will teleported to areatrigger target point.'),
('trigger active',2,'Syntax: .trigger active\r\n\r\nShow list of areatriggers with activation zone including current character position.');
