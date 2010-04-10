ALTER TABLE db_version CHANGE COLUMN required_9710_01_mangos_command required_9716_01_mangos_npc_vendor bit;

ALTER TABLE npc_vendor
  DROP PRIMARY KEY,
  ADD PRIMARY KEY  (`entry`,`item`,`ExtendedCost`);
