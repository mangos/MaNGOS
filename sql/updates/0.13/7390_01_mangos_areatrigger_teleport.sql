ALTER TABLE db_version CHANGE COLUMN required_7388_01_mangos_mangos_string required_7390_01_mangos_areatrigger_teleport bit;

ALTER TABLE areatrigger_teleport
  ADD COLUMN required_quest_done_heroic int(11) unsigned NOT NULL default '0' AFTER required_quest_done;