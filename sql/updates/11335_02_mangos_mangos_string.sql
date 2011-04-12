ALTER TABLE db_version CHANGE COLUMN required_11335_01_mangos_mangos_string required_11335_02_mangos_mangos_string bit;

REPLACE INTO mangos_string VALUES
(503,'The distance is: (3D) %f (2D) %f - (3D, point-to-point) %f yards.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
