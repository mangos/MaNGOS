ALTER TABLE realmd_db_version CHANGE COLUMN required_8728_01_realmd_account required_9010_01_realmd_realmlist bit;

ALTER TABLE realmlist
  ADD COLUMN realmbuilds varchar(64) NOT NULL default '' AFTER population;
