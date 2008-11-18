ALTER TABLE db_version CHANGE COLUMN required_2008_11_16_01_mangos_command required_2008_11_18_01_mangos_creature_movement bit;

ALTER TABLE creature_movement
  DROP `text1`,
  DROP `text2`,
  DROP `text3`,
  DROP `text4`,
  DROP `text5`,
  ADD COLUMN textid1 int(11) NOT NULL default '0' AFTER waittime,
  ADD COLUMN textid2 int(11) NOT NULL default '0' AFTER textid1,
  ADD COLUMN textid3 int(11) NOT NULL default '0' AFTER textid2,
  ADD COLUMN textid4 int(11) NOT NULL default '0' AFTER textid3,
  ADD COLUMN textid5 int(11) NOT NULL default '0' AFTER textid4;
