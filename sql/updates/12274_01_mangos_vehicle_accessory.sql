ALTER TABLE db_version CHANGE COLUMN required_12257_01_mangos_spell_learn_spell required_12274_01_mangos_vehicle_accessory bit;

--
-- Table structure for table `vehicle_accessory`
--
DROP TABLE IF EXISTS vehicle_accessory;
CREATE TABLE `vehicle_accessory` (
  `vehicle_entry` int(10) UNSIGNED NOT NULL COMMENT 'entry of the npc who has some accessory as vehicle',
  `seat` mediumint(8) UNSIGNED NOT NULL COMMENT 'onto which seat shall the passenger be boarded',
  `accessory_entry` int(10) UNSIGNED NOT NULL COMMENT 'entry of the passenger that is to be boarded',
  `comment` varchar(255) NOT NULL,
  PRIMARY KEY  (`vehicle_entry`, `seat`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Vehicle Accessory (passengers that are auto-boarded onto a vehicle)';
