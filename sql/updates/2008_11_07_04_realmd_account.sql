ALTER TABLE realmd_db_version CHANGE COLUMN required_2008_11_07_02_realmd_realmd_db_version required_2008_11_07_04_realmd_account bit;

ALTER TABLE `account`
  CHANGE COLUMN  `email` `email` text;

