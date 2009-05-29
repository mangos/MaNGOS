ALTER TABLE db_version CHANGE COLUMN required_7908_02_mangos_creature_addon required_7908_03_mangos_creature_template_addon bit;

ALTER TABLE creature_template_addon
  DROP COLUMN bytes0;
