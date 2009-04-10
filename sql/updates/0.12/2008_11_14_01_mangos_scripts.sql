ALTER TABLE db_version CHANGE COLUMN required_2008_11_11_02_mangos_scripts required_2008_11_14_01_mangos_scripts bit;

ALTER TABLE event_scripts
  CHANGE COLUMN dataint dataint int(11) NOT NULL default '0' AFTER datalong2;

ALTER TABLE gameobject_scripts
  CHANGE COLUMN dataint dataint int(11) NOT NULL default '0' AFTER datalong2;

ALTER TABLE quest_end_scripts
  CHANGE COLUMN dataint dataint int(11) NOT NULL default '0' AFTER datalong2;

ALTER TABLE quest_start_scripts
  CHANGE COLUMN dataint dataint int(11) NOT NULL default '0' AFTER datalong2;

ALTER TABLE spell_scripts
  CHANGE COLUMN dataint dataint int(11) NOT NULL default '0' AFTER datalong2;
