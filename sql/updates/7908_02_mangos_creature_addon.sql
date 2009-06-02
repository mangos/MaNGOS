ALTER TABLE db_version CHANGE COLUMN required_7908_01_mangos_creature_template required_7908_02_mangos_creature_addon bit;

ALTER TABLE creature_addon
  DROP COLUMN bytes0;
