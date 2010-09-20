ALTER TABLE `spell_pet_auras` DROP PRIMARY KEY;
ALTER TABLE `spell_pet_auras` ADD PRIMARY KEY ( `spell` , `effectId` , `pet` , `aura` );

-- DK Ghoul
DELETE FROM `spell_pet_auras` WHERE `aura` = 54566;
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 26125;
INSERT INTO `spell_pet_auras` VALUES
-- (0,0,26125,62137),
(0,0,26125,51996),
(0,0,26125,54566),
(0,0,26125,61697);

-- Hunter pets
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 1;
INSERT INTO `spell_pet_auras` VALUES
-- (0,0,1,65220),
(0,0,1,34902),
(0,0,1,34903),
(0,0,1,34904),
(0,0,1,61017);

-- Warlock demons
-- imp
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 416;
INSERT INTO `spell_pet_auras` VALUES
-- (0,0,416,32233),
(0,0,416,34947),
(0,0,416,34956),
(0,0,416,34957),
(0,0,416,34958),
(0,0,416,61013);

-- felhunter
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 417;
INSERT INTO `spell_pet_auras` VALUES
-- (0,0,417,32233),
(0,0,417,34947),
(0,0,417,34956),
(0,0,417,34957),
(0,0,417,34958),
(0,0,417,61013);

-- succubus
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 1863;
INSERT INTO `spell_pet_auras` VALUES
-- (0,0,1863,32233),
(0,0,1863,34947),
(0,0,1863,34956),
(0,0,1863,34957),
(0,0,1863,34958),
(0,0,1863,61013);

-- felguard
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 17252;
INSERT INTO `spell_pet_auras` VALUES
-- (0,0,17252,32850),
-- (0,0,17252,32233),
(0,0,17252,34947),
(0,0,17252,34956),
(0,0,17252,34957),
(0,0,17252,34958),
(0,0,17252,61013);

-- Shaman wolf
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 29264;
INSERT INTO `spell_pet_auras` VALUES
(0,0,29264,34902),
(0,0,29264,34903),
(0,0,29264,34904),
(0,0,29264,61783);

-- Shadowfiend
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 19668;
INSERT INTO `spell_pet_auras` VALUES
(0,0,19668,34947),
(0,0,19668,34902);

-- Mage water elemental
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 37994;
INSERT INTO `spell_pet_auras` VALUES
(0,0,37994,34947),
(0,0,37994,34956);
