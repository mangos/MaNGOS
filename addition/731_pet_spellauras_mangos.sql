ALTER TABLE `spell_pet_auras` DROP PRIMARY KEY;
ALTER TABLE `spell_pet_auras` ADD PRIMARY KEY ( `spell` , `effectId` , `pet` , `aura` );

-- DK Ghoul
DELETE FROM `spell_pet_auras` WHERE `aura` = 54566;
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 26125;
INSERT INTO `spell_pet_auras` VALUES
(0,0,26125,51996),
(0,0,26125,54566),
(0,0,26125,61697);

-- Hunter pets
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 1;
INSERT INTO `spell_pet_auras` VALUES
(0,0,1,34902),
(0,0,1,34903),
(0,0,1,34904),
(0,0,1,61017);

-- Ferocious Inspiration
DELETE FROM `spell_pet_auras` WHERE `spell` IN ('34455','34459','34460');
INSERT INTO `spell_pet_auras` (`spell`, `effectId`, `pet`, `aura`) VALUES
('34455','0','0','75593'),
('34459','0','0','75446'),
('34460','0','0','75447');

-- Warlock demons
-- imp
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 416;
INSERT INTO `spell_pet_auras` VALUES
(0,0,416,34947),
(0,0,416,34956),
(0,0,416,34957),
(0,0,416,34958),
(0,0,416,61013);

-- felhunter
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 417;
INSERT INTO `spell_pet_auras` VALUES
(0,0,417,34947),
(0,0,417,34956),
(0,0,417,34957),
(0,0,417,34958),
(0,0,417,61013);

-- succubus
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 1863;
INSERT INTO `spell_pet_auras` VALUES
(0,0,1863,34947),
(0,0,1863,34956),
(0,0,1863,34957),
(0,0,1863,34958),
(0,0,1863,61013);

-- felguard
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 17252;
INSERT INTO `spell_pet_auras` VALUES
(0,0,17252,34947),
(0,0,17252,34956),
(0,0,17252,34957),
(0,0,17252,34958),
(0,0,17252,61013);

-- Shaman wolf
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 29264;
INSERT INTO `spell_pet_auras` VALUES
(0,0,29264,58877),
(0,0,29264,34902),
(0,0,29264,34903),
(0,0,29264,34904),
(0,0,29264,61783);

-- Shadowfiend
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 19668;
INSERT INTO `spell_pet_auras` VALUES
(0,0,19668,34947);

-- Mage water elemental
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 37994;
INSERT INTO `spell_pet_auras` VALUES
(0,0,37994,34947),
(0,0,37994,34956);

-- Mage water elemental (old guardian)
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 510;
INSERT INTO `spell_pet_auras` VALUES
(0,0,510,34947),
(0,0,510,34956);

-- Mage mirror image
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 31216;
INSERT INTO `spell_pet_auras` VALUES
(0,0,31216,34947);

-- DK Bloodworms
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 28017;
INSERT INTO `spell_pet_auras` VALUES
(0,0,28017,50453);

-- DK Gargoyle
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 27829;
INSERT INTO `spell_pet_auras` VALUES
(0,0,27829,54566);

-- Greater Earth Elemetal
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 15352;
INSERT INTO `spell_pet_auras` VALUES
(0,0,15352, 7941),
(0,0,15352,34947);

-- Greater Fire Elemetal
DELETE FROM `spell_pet_auras` WHERE `spell` = 0 AND `pet` = 15438;
INSERT INTO `spell_pet_auras` VALUES
(0,0,15438, 7942),
(0,0,15438,34956),
(0,0,15438,34947);

-- Need correct spellcasting for this!
-- UPDATE `creature_template` SET `spell1` = 12470, `spell2` = 57984 WHERE `entry` = 15438;
-- UPDATE `creature_template` SET `spell1` = 36213 WHERE `entry` = 15352;

UPDATE `creature_template` SET `spell1` = 0, `spell2` = 0 WHERE `entry` = 15438;
UPDATE `creature_template` SET `spell1` = 0, `spell2` = 0 WHERE `entry` = 15352;


UPDATE `creature_template` SET `spell1` = 40133 WHERE `entry` = 15439;
UPDATE `creature_template` SET `spell1` = 40132 WHERE `entry` = 15430;

DELETE FROM `event_scripts` WHERE `id` IN (14859,14858);
INSERT INTO `event_scripts` (`id`, `delay`, `command`, `datalong`, `datalong2`, `datalong3`, `datalong4`, `data_flags`, `dataint`, `dataint2`, `dataint3`, `dataint4`, `x`, `y`, `z`, `o`, `comments`) VALUES
(14858, 1, 15, 33663, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Summon greater Earth elemental'),
(14859, 1, 15, 32982, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Summon greater Fire  elemental');
