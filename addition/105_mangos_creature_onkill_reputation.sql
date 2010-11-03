ALTER TABLE `creature_onkill_reputation` 
 ADD COLUMN `ChampioningAura` int(11) unsigned NOT NULL default '0' AFTER `TeamDependent`;
