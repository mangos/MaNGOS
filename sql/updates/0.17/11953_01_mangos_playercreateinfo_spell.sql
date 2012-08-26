ALTER TABLE db_version CHANGE COLUMN required_11947_01_mangos_dbscripts required_11953_01_mangos_playercreateinfo_spell bit;

DELETE FROM playercreateinfo_spell WHERE Spell=21009;
INSERT INTO playercreateinfo_spell (race,class,Spell,Note) VALUES
(4,6,21009,'Elusiveness'),
(4,11,21009,'Elusiveness');
