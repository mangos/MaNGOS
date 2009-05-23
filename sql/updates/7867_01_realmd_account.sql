ALTER TABLE realmd_db_version CHANGE COLUMN required_7546_02_realmd_uptime required_7867_01_realmd_account bit;

ALTER TABLE `account` CHANGE COLUMN `last_ip` `last_ip` varchar(30) NOT NULL default '0.0.0.0';
ALTER TABLE `ip_banned` CHANGE COLUMN `ip` `ip` varchar(32) NOT NULL default '0.0.0.0';
