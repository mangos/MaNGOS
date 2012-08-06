ALTER TABLE db_version CHANGE COLUMN required_10197_01_mangos_playercreateinfo required_10203_01_mangos_item_template bit;

ALTER TABLE item_template
  CHANGE COLUMN Faction Flags2 int(10) unsigned NOT NULL default '0';

