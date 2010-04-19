-- Unholy Blight

DELETE FROM `spell_proc_event` WHERE `entry` = 49194;
INSERT INTO `spell_proc_event` VALUES
(49194,0x00,15,0x00002000,0x00000000,0x00000000,0x00000000,0x00000000,0.000000,0.000000,0);

-- Stoneclaw Totem

UPDATE `creature_template` SET `spell1` = 55328 WHERE `entry` = 3579;
UPDATE `creature_template` SET `spell1` = 55329 WHERE `entry` = 3911;
UPDATE `creature_template` SET `spell1` = 55330 WHERE `entry` = 3912;
UPDATE `creature_template` SET `spell1` = 55332 WHERE `entry` = 3913;
UPDATE `creature_template` SET `spell1` = 55333 WHERE `entry` = 7398;
UPDATE `creature_template` SET `spell1` = 55335 WHERE `entry` = 7399;
UPDATE `creature_template` SET `spell1` = 55278 WHERE `entry` = 15478;
UPDATE `creature_template` SET `spell1` = 58589 WHERE `entry` = 31120;
UPDATE `creature_template` SET `spell1` = 58590 WHERE `entry` = 31121;
UPDATE `creature_template` SET `spell1` = 58591 WHERE `entry` = 31122;

-- King of the Jungle

DELETE FROM `spell_proc_event` WHERE `entry` IN (48492,48494,48495);
INSERT INTO `spell_proc_event` VALUES 
(48492,0,7,524288,0,2048,16384,0,0,0,0),
(48494,0,7,524288,0,2048,16384,0,0,0,0),
(48495,0,7,524288,0,2048,16384,0,0,0,0);

-- Moonkin Form 

DELETE FROM `spell_proc_event` WHERE `entry` = 24905;
INSERT INTO `spell_proc_event` VALUES
(24905,0,0,0,0,0,0,2,0,50,0);

-- Blood Plague

UPDATE `spell_bonus_data` SET `dot_bonus` = 1.15 WHERE entry IN (55078,55095);

-- Impact

DELETE FROM `spell_chain` WHERE `first_spell` = 11103;
INSERT INTO `spell_chain` VALUES
(11103,0,11103,1,0),
(12357,11103,11103,2,0),
(12358,12357,11103,3,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (11103,12357,12358,64343);
INSERT INTO `spell_proc_event` VALUES
(11103,0x00000000,3,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0.0,0.0,0),
(64343,0x00000000,3,0x00000002,0x00000000,0x00000000,0x00000000,0x00000000,0.0,0.0,0);

-- Mind Sear

UPDATE `spell_bonus_data` SET `direct_bonus` = 0.286 WHERE `entry` = 49821;

-- Enrage

DELETE FROM `spell_proc_event` WHERE `entry` IN (12317,13045,13046,13047,13048);

-- Unbridled Wrath

update `spell_proc_event` set `ppmRate` = 3 where `entry` = 12322;
update `spell_proc_event` set `ppmRate` = 6 where `entry` = 12999;
update `spell_proc_event` set `ppmRate` = 9 where `entry` = 13000;
update `spell_proc_event` set `ppmRate` = 12 where `entry` = 13001;
update `spell_proc_event` set `ppmRate` = 15 where `entry` = 13002;

-- Lock and Load

DELETE FROM `spell_chain` WHERE spell_id IN (56342,56343,56344);
INSERT INTO `spell_chain` VALUES
(56342,0,56342,1,0),
(56343,56342,56342,2,0),
(56344,56343,56342,3,0);

DELETE FROM `spell_proc_event` WHERE entry IN (56342,56343,56344);
INSERT INTO `spell_proc_event` VALUES
(56342,0,9,0x0C,0x8000000,0x60000,0,0,0,0,22);

-- Sacred Cleansing

DELETE FROM `spell_chain` WHERE `first_spell` = 53551;
INSERT INTO `spell_chain` VALUES
(53551,0,53551,1,0),
(53552,53551,53551,2,0),
(53553,53552,53551,3,0);

DELETE FROM `spell_proc_event` WHERE `entry` IN (53551,53552,53553);
INSERT INTO `spell_proc_event` VALUES
(53551,0x00000000,10,0x00001000,0x00000000,0x00000000,0x00000000,0x00000000,0.0,0.0,0);

-- Item - Druid T9 Restoration Relic 

DELETE FROM `spell_proc_event` WHERE `entry` = 67356;
INSERT INTO `spell_proc_event` VALUES (67356,0x00,7,0x00000010,0x00000000,0x00000000,0x00000000,0x00000000,0.000000,0.000000,0);
