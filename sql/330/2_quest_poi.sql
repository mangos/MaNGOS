DROP TABLE IF EXISTS `quest_poi`;
CREATE TABLE `quest_poi` (
  `questid` int(11) unsigned NOT NULL DEFAULT '0',
  `objIndex` int(11) NOT NULL DEFAULT '0',
  `mapId` int(11) unsigned NOT NULL DEFAULT '0',
  `unk1` int(11) unsigned NOT NULL DEFAULT '0',
  `unk2` int(11) unsigned NOT NULL DEFAULT '0',
  `unk3` int(11) unsigned NOT NULL DEFAULT '0',
  `unk4` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`questid`,`objIndex`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `quest_poi_points`;
CREATE TABLE `quest_poi_points` (
  `questId` int(11) unsigned NOT NULL DEFAULT '0',
  `objIndex` int(11) NOT NULL DEFAULT '0',
  `x` int(11) NOT NULL DEFAULT '0',
  `y` int(11) NOT NULL DEFAULT '0',
  KEY `idx` (`questId`,`objIndex`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Sample data for quest 456
INSERT INTO quest_poi VALUES (456, 0, 1, 0, 0, 0, 0);
INSERT INTO quest_poi VALUES (456, 1, 1, 0, 0, 0, 0);
INSERT INTO quest_poi VALUES (456, -1, 1, 0, 0, 0, 0);

INSERT INTO quest_poi_points VALUES (456, 0, 0x28CF, 0x0217);
INSERT INTO quest_poi_points VALUES (456, 0, 0x29F4, 0x02AA);
INSERT INTO quest_poi_points VALUES (456, 0, 0x2841, 0x0398);
INSERT INTO quest_poi_points VALUES (456, 0, 0x2806, 0x034C);
INSERT INTO quest_poi_points VALUES (456, 0, 0x281B, 0x02DE);
INSERT INTO quest_poi_points VALUES (456, 0, 0x283C, 0x029B);
INSERT INTO quest_poi_points VALUES (456, 0, 0x284C, 0x028A);
INSERT INTO quest_poi_points VALUES (456, 0, 0x28B0, 0x0228);

INSERT INTO quest_poi_points VALUES (456, 1, 0x28A0, 0x0258);
INSERT INTO quest_poi_points VALUES (456, 1, 0x290E, 0x0366);
INSERT INTO quest_poi_points VALUES (456, 1, 0x28CA, 0x03BC);
INSERT INTO quest_poi_points VALUES (456, 1, 0x288F, 0x03F6);
INSERT INTO quest_poi_points VALUES (456, 1, 0x284D, 0x03B8);
INSERT INTO quest_poi_points VALUES (456, 1, 0x2828, 0x0395);
INSERT INTO quest_poi_points VALUES (456, 1, 0x2806, 0x034C);
INSERT INTO quest_poi_points VALUES (456, 1, 0x281B, 0x02DE);
INSERT INTO quest_poi_points VALUES (456, 1, 0x284C, 0x028A);

INSERT INTO quest_poi_points VALUES (456, -1, 0x2859, 0x033A);
