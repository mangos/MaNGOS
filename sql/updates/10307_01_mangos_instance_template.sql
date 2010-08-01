ALTER TABLE db_version CHANGE COLUMN required_10299_01_mangos_event_id_scripts required_10307_01_mangos_instance_template bit;

ALTER TABLE instance_template CHANGE COLUMN `script` `ScriptName` varchar(128) NOT NULL default '';
