ALTER TABLE db_version CHANGE COLUMN required_2008_11_09_02_mangos_command required_2008_11_09_03_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (453);
