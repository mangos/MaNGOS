ALTER TABLE db_version CHANGE COLUMN required_12225_02_mangos_quest_template required_12241_01_mangos_creature_template bit;

ALTER TABLE `creature_template` ADD COLUMN `unit_flags2` int(10) unsigned NOT NULL default '0' AFTER `unit_flags`;
