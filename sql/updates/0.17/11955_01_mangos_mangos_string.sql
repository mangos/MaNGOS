ALTER TABLE db_version CHANGE COLUMN required_11953_01_mangos_playercreateinfo_spell required_11955_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (555,556);
