ALTER TABLE db_version CHANGE COLUMN required_2008_10_29_05_mangos_command required_2008_10_31_01_mangos_creature_template bit;

ALTER TABLE `creature_template`
  CHANGE COLUMN `flags` `unit_flags` int(10) unsigned NOT NULL default '0',
  CHANGE COLUMN `flag1` `type_flags` int(10) unsigned NOT NULL default '0';
