ALTER TABLE db_version CHANGE COLUMN required_12112_03_mangos_gameobject_template required_12112_04_mangos_npc_vendor bit;

ALTER TABLE `npc_vendor` MODIFY COLUMN `item` mediumint(8) NOT NULL DEFAULT '0';
ALTER TABLE `npc_vendor` MODIFY COLUMN `maxcount` smallint(5) unsigned NOT NULL default '0';
