ALTER TABLE realmd_db_version CHANGE COLUMN required_9010_01_realmd_realmlist required_9746_01_realmd_realmlist bit;

ALTER TABLE realmlist
  CHANGE COLUMN color realmflags tinyint(3) unsigned NOT NULL default '0'
  COMMENT 'Supported masks: 0x1 (invalid, not show in realm list), 0x4 (show version and build), 0x20 (new players), 0x40 (recommended)';


UPDATE realmlist
  SET realmflags = realmflags & ~(0x01 | 0x04 | 0x20 | 0x40) ;
