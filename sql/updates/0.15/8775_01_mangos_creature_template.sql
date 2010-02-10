ALTER TABLE db_version CHANGE COLUMN required_8770_01_mangos_quest_template required_8775_01_mangos_creature_template bit;

ALTER TABLE `creature_template` ADD `difficulty_entry_2` MEDIUMINT(8) unsigned
 NOT NULL default 0 AFTER `difficulty_entry_1`;
ALTER TABLE `creature_template` ADD `difficulty_entry_3` MEDIUMINT(8) unsigned
 NOT NULL default 0 AFTER `difficulty_entry_2`;
