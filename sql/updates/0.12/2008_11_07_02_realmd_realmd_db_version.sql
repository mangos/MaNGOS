--
-- Table structure for table `realmd_db_version`
--

DROP TABLE IF EXISTS `realmd_db_version`;
CREATE TABLE `realmd_db_version` (
  `required_2008_11_07_02_realmd_realmd_db_version` bit(1) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Last applied sql update to DB';

--
-- Dumping data for table `realmd_db_version`
--

LOCK TABLES `realmd_db_version` WRITE;
/*!40000 ALTER TABLE `realmd_db_version` DISABLE KEYS */;
INSERT INTO `realmd_db_version` VALUES
(NULL);
/*!40000 ALTER TABLE `realmd_db_version` ENABLE KEYS */;
UNLOCK TABLES;
