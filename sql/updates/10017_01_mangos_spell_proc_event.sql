ALTER TABLE db_version CHANGE COLUMN required_10015_01_mangos_spell_proc_event required_10017_01_mangos_spell_proc_event bit;

ALTER TABLE spell_proc_event
  CHANGE COLUMN `SchoolMask` `SchoolMask` tinyint(4) unsigned NOT NULL default '0';


