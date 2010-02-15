ALTER TABLE db_version CHANGE COLUMN required_9382_01_mangos_command required_9385_01_mangos_command bit;

DELETE FROM command WHERE name IN ('reload all_scripts','reload all_achievement','reload all_item','reload all_eventai','reload all_npc');
INSERT INTO command VALUES
('reload all_achievement',3,'Syntax: .reload all_achievement\r\n\r\nReload all `achievement_*` tables if reload support added for this table and this table can be _safe_ reloaded.'),
('reload all_eventai',3,'Syntax: .reload all_eventai\r\n\r\nReload `creature_ai_*` tables if reload support added for these tables and these tables can be _safe_ reloaded.'),
('reload all_item',3,'Syntax: .reload all_item\r\n\r\nReload `item_required_target`, `page_texts` and `item_enchantment_template` tables.'),
('reload all_npc',3,'Syntax: .reload all_npc\r\n\r\nReload `points_of_interest` and `npc_*` tables if reload support added for these tables and these tables can be _safe_ reloaded.'),
('reload all_scripts',3,'Syntax: .reload all_scripts\r\n\r\nReload `*_scripts` tables.');
