ALTER TABLE db_version CHANGE COLUMN required_12012_01_mangos_spell_template required_12020_01_mangos_command bit;

DELETE FROM command WHERE name LIKE 'honor addkill';
INSERT INTO command VALUES ('honor addkill',2,'Syntax: .honor addkill\r\n\r\nAdd the targeted unit as one of your pvp kills today (you only get honor if it\'s a racial leader or a player)');
