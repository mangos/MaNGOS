ALTER TABLE db_version CHANGE COLUMN required_11690_01_mangos_spell_proc_event required_11701_01_mangos_command bit;

DELETE FROM command WHERE name = 'auction item';

INSERT INTO command (name, security, help) VALUES
('auction item',3,'Syntax: .auction item (alliance|horde|goblin) #itemid[:#itemcount] [[[#minbid] #buyout] [short|long|verylong]\r\n\r\nAdd new item (in many stackes if amount grater stack size) to specific auction house at short|long|verylogn perios similar same settings in auction in game dialog. Created auction not have owner.');
