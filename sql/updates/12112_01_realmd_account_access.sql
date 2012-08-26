ALTER TABLE realmd_db_version CHANGE COLUMN required_10008_01_realmd_realmd_db_version required_12112_01_realmd_account_access bit;

ALTER TABLE `account` DROP `gmlevel`;

DROP TABLE IF EXISTS `account_access`;
CREATE TABLE `account_access` (
  `id` int(10) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL,
  `RealmID` int(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Account Access System';

INSERT INTO `account_access` VALUES
(1,3,-1),
(2,2,-1),
(3,1,-1),
(4,0,-1);
