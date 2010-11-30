-- Pet scaling data table from /dev/rsa
DROP TABLE IF EXISTS `pet_scaling_data`;
CREATE TABLE IF NOT EXISTS `pet_scaling_data` (
  `creature_entry` mediumint(8) unsigned NOT NULL,
  `aura` mediumint(8) unsigned NOT NULL default '0',
  `healthbase` mediumint(8) NOT NULL default '0',
  `health` mediumint(8) NOT NULL default '0',
  `powerbase` mediumint(8) NOT NULL default '0',
  `power` mediumint(8) NOT NULL default '0',
  `str` mediumint(8) NOT NULL default '0',
  `agi` mediumint(8) NOT NULL default '0',
  `sta` mediumint(8) NOT NULL default '0',
  `inte` mediumint(8) NOT NULL default '0',
  `spi` mediumint(8) NOT NULL default '0',
  `armor` mediumint(8) NOT NULL default '0',
  `resistance1` mediumint(8) NOT NULL default '0',
  `resistance2` mediumint(8) NOT NULL default '0',
  `resistance3` mediumint(8) NOT NULL default '0',
  `resistance4` mediumint(8) NOT NULL default '0',
  `resistance5` mediumint(8) NOT NULL default '0',
  `resistance6` mediumint(8) NOT NULL default '0',
  `apbase` mediumint(8) NOT NULL default '0',
  `apbasescale` mediumint(8) NOT NULL default '0',
  `attackpower` mediumint(8) NOT NULL default '0',
  `damage` mediumint(8) NOT NULL default '0',
  `spelldamage` mediumint(8) NOT NULL default '0',
  `spellhit` mediumint(8) NOT NULL default '0',
  `hit` mediumint(8) NOT NULL default '0',
  `expertize` mediumint(8) NOT NULL default '0',
  `attackspeed` mediumint(8) NOT NULL default '0',
  `crit` mediumint(8) NOT NULL default '0',
  `regen` mediumint(8) NOT NULL default '0',
  PRIMARY KEY  (`creature_entry`,`aura`)
) DEFAULT CHARSET=utf8 PACK_KEYS=0 COMMENT='Stores pet scaling data (in percent from owner value).';

-- Creature 0 (default) - MUST be exist!
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 0;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(0, 0, 0, 1000, 0, 1500, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 200, 0, 0, 0, 100, 100, 100, 100, 0, 0);

-- Pet 1 - default hunter pet
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 1;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(1,     0, 0, 1050, 0, 1500, 0, 0, 45, 0, 0, 35, 40, 40, 40, 40, 40, 40, 20, 200, 22, 0, 13, 100, 100, 100, 100, 0, 0),
(1, 62758, 0,    0, 0,    0, 0, 0,  9, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  6, 0,  3,   0,   0,   0,   0, 0, 0),
(1, 62762, 0,    0, 0,    0, 0, 0, 18, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,   0, 12, 0,  6,   0,   0,   0,   0, 0, 0);

-- Pet 26611 - shaman spirit wolf
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 29264;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(29264,     0, 0, 1000, 0, 1500, 0, 0, 30, 0, 0, 35, 40, 40, 40, 40, 40, 40, 20, 200, 22, 0, 13, 100, 100, 100, 100, 0, 0),
(29264, 63271, 0,    0, 0,    0, 0, 0, 30, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,   0, 0,  0,  0,   0,   0,   0,   0, 0, 0);

-- Pet 26125 - DK ghoul
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 26125;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(26125,     0, 0, 1000, 0, 0, 70, 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 200, 0, 0, 0, 0, 100, 100, 100, 0, 0),
(26125, 48965, 0,    0, 0, 0, 14, 0,  6, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,   0, 0, 0, 0, 0,   0,   0,   0, 0, 0),
(26125, 49571, 0,    0, 0, 0, 28, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,   0, 0, 0, 0, 0,   0,   0,   0, 0, 0),
(26125, 49572, 0,    0, 0, 0, 42, 0, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,   0, 0, 0, 0, 0,   0,   0,   0, 0, 0),
(26125, 58721, 0,    0, 0, 0, 40, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,   0, 0, 0, 0, 0,   0,   0,   0, 0, 0);

-- Pet 416 - warlock imp
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 416;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(416, 0, 0, 840, 0, 495, 0, 0, 75, 30, 0, 35, 40, 40, 40, 40, 40, 40, 10, 100, 57, 0, 15, 100, 100, 100, 100, 0, 20);

-- Pet 417 - warlock felhunter
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 417;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(417, 0, 0, 950, 0, 1150, 0, 0, 75, 30, 0, 35, 40, 40, 40, 40, 40, 40, 20, 200, 57, 0, 15, 100, 100, 100, 100, 0, 20);

-- Pet 1860 - warlock voidwalker
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 1860;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(1860,     0, 0, 1100, 0, 1150, 0, 0, 75, 30, 0, 35, 40, 40, 40, 40, 40, 40, 20, 200, 57, 0, 15, 100, 100, 100, 100, 0, 20),
(1860, 57277, 0,    0, 0,    0, 0, 0, 20,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,   0,  0, 0,  0,   0,   0,   0,   0, 0,  0);

-- Pet 1863 - warlock succubus
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 1863;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(1863, 0, 0, 910, 0, 1150, 0, 0, 75, 30, 0, 35, 40, 40, 40, 40, 40, 40, 20, 200, 57, 0, 15, 100, 100, 100, 100, 0, 20);

-- Pet 17252 - warlock felguard
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 17252;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(17252,     0, 0, 1100, 0, 1150, 0, 0, 75, 30, 0, 35, 40, 40, 40, 40, 40, 40, 20, 200, 57, 0, 15, 100, 100, 100, 100, 0, 20),
(17252, 56246, 0,    0, 0,    0, 0, 0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,   0, 20, 0,  0,   0,   0,   0,   0, 0,  0);


-- Pet 37994 - Mage water elemental
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 37994;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(37994, 0, 0, 1000, 0, 1500, 0, 0, 30, 30, 0, 35, 0, 0, 0, 0, 0, 0, 20, 200, 0, 0, 40, 100, 100, 100, 100, 0, 0);

-- Pet 19668 - Priest  Shadowfiend
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 19668;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(19668, 0, 0, 1000, 0, 1500, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 200, 400, 0, 67, 100, 100, 100, 100, 0, 0);

-- Guardian 31216 - mage Mirror image
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 31216;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(31216, 0, 0, 1000, 0, 1500, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 200, 0, 0, 33, 100, 100, 100, 100, 0, 0);

-- Guardian 27829 - DK Gargoyle
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 27829;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(27829, 0, 0, 1000, 0, 1500, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 200, 0, 0, 50, 100, 100, 100, 100, 0, 0);

-- Guardian 15352 - Greater Earth Elemental
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 15352;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(15352, 0, 0, 1000, 0, 1500, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 200, 40, 0, 30, 100, 100, 100, 100, 0, 0);

-- Guardian 15438 - Greater Fire Elemental
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 15438;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(15438, 0, 0, 1000, 0, 1500, 0, 0, 20, 10, 0, 0, 0, 0, 0, 0, 0, 0, 20, 200, 80, 0, 40, 100, 100, 100, 100, 0, 0);

-- Pet 510 - Mage water elemental (old guardian)
DELETE FROM `pet_scaling_data` WHERE `creature_entry` = 510;
INSERT INTO `pet_scaling_data` (`creature_entry`, `aura`, `healthbase`, `health`, `powerbase`, `power`, `str`, `agi`, `sta`, `inte`, `spi`, `armor`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `apbase`, `apbasescale`, `attackpower`, `damage`, `spelldamage`, `spellhit`, `hit`, `expertize`, `attackspeed`, `crit`, `regen`) VALUES
(510, 0, 0, 1000, 0, 1500, 0, 0, 30, 30, 0, 35, 0, 0, 0, 0, 0, 0, 20, 200, 0, 0, 40, 100, 100, 100, 100, 0, 0);

