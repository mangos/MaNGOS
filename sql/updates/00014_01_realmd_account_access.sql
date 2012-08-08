ALTER TABLE realmd_db_version CHANGE COLUMN required_0001_xxxxx_01_realmd required_00014_01_realmd_account_access bit;

--
-- Table Stucture for table `account `account_access`
--

DROP TABLE IF EXISTS `account_access`;

CREATE TABLE `account_access` (
  `id` int(10) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL,
  `RealmID` int(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
