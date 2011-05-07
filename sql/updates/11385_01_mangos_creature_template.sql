ALTER TABLE db_version CHANGE COLUMN required_11348_01_mangos_spell_bonus_data required_11385_01_mangos_creature_template bit;

ALTER TABLE creature_template
  ADD COLUMN `vehicle_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER `RegenHealth`;
