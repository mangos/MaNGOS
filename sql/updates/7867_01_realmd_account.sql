ALTER TABLE `account` CHANGE COLUMN `last_ip` `last_ip` varchar(30) NOT NULL default '0.0.0.0';
ALTER TABLE `ip_banned` CHANGE COLUMN `ip` `ip` varchar(32) NOT NULL default '0.0.0.0';
