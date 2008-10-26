ALTER TABLE db_version CHANGE COLUMN required_2008_10_29_04_mangos_mangos_string required_2008_10_29_05_mangos_command bit;

DELETE FROM command WHERE name IN ('npc follow','npc unfollow','waterwalk');

INSERT INTO command VALUES
('npc follow',2,'Syntax: .npc follow\r\n\r\nSelected creature start follow you until death/fight/etc.'),
('npc unfollow',2,'Syntax: .npc unfollow\r\n\r\nSelected creature (non pet) stop follow you.'),
('waterwalk',2,'Syntax: .waterwalk on/off\r\n\r\nSet on/off waterwalk state for selected player.');
