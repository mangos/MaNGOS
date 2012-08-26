ALTER TABLE db_version CHANGE COLUMN required_10244_01_mangos_command required_10251_01_mangos_command bit;

DELETE FROM command WHERE name = 'wp';
