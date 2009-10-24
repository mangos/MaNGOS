ALTER TABLE realmd_db_version CHANGE COLUMN required_8332_01_realmd_realmcharacters required_8728_01_realmd_account bit;

ALTER TABLE account
  DROP COLUMN online,
  ADD COLUMN  active_realm_id int(11) unsigned NOT NULL default '0' AFTER last_login;
