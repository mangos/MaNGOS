ALTER TABLE db_version CHANGE COLUMN required_2008_10_31_02_mangos_mangos_string required_2008_10_31_03_mangos_command bit;

DELETE FROM command WHERE name IN ('npc tame');

INSERT INTO command VALUES
('npc tame',2,'Syntax: .npc tame\r\n\r\nTame selected creature (tameable non pet creature). You don''t must have pet.');
