ALTER TABLE db_version CHANGE COLUMN required_10270_01_mangos_reputation_spillover_template required_10286_01_mangos_creature_addon bit;

ALTER TABLE creature_addon
  CHANGE `guid` `guid` int(10) unsigned NOT NULL default '0';
