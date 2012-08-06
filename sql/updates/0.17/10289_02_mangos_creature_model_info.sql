ALTER TABLE db_version CHANGE COLUMN required_10289_01_mangos_creature_template required_10289_02_mangos_creature_model_info bit;

ALTER TABLE creature_model_info ADD COLUMN modelid_alternative mediumint(8) unsigned NOT NULL default '0' AFTER modelid_other_gender;
ALTER TABLE creature_model_info ADD COLUMN modelid_other_team mediumint(8) unsigned NOT NULL default '0' AFTER modelid_alternative;
