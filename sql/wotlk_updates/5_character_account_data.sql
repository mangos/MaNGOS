CREATE TABLE `account_data` (
  `account` int(11) unsigned NOT NULL default '0',
  `type` int(11) unsigned NOT NULL default '0',
  `time` bigint(11) unsigned NOT NULL default '0',
  `data` longtext NOT NULL,
  PRIMARY KEY  (`account`,`type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
