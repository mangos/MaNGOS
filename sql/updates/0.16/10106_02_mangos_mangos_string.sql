ALTER TABLE db_version CHANGE COLUMN required_10106_01_mangos_command required_10106_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (269);
