ALTER TABLE db_version CHANGE COLUMN required_10867_01_mangos_npc_trainer_template required_10867_02_mangos_creature_template bit;

ALTER TABLE creature_template
  ADD COLUMN trainer_id mediumint(8) unsigned NOT NULL default '0' AFTER equipment_id;
