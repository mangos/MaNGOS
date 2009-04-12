ALTER TABLE db_version CHANGE COLUMN required_2008_11_11_01_mangos_db_script_string required_2008_11_11_02_mangos_scripts bit;

ALTER TABLE event_scripts
  DROP datatext,
  ADD COLUMN dataint int(11) NOT NULL default '0';

ALTER TABLE gameobject_scripts
  DROP datatext,
  ADD COLUMN dataint int(11) NOT NULL default '0';

ALTER TABLE quest_end_scripts
  DROP datatext,
  ADD COLUMN dataint int(11) NOT NULL default '0';

ALTER TABLE quest_start_scripts
  DROP datatext,
  ADD COLUMN dataint int(11) NOT NULL default '0';

ALTER TABLE spell_scripts
  DROP datatext,
  ADD COLUMN dataint int(11) NOT NULL default '0';
