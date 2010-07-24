ALTER TABLE db_version CHANGE COLUMN required_9752_01_mangos_gameobject_template required_9753_01_mangos_instance_template bit;

ALTER TABLE instance_template CHANGE COLUMN parent parent smallint(5) unsigned NOT NULL default '0';
