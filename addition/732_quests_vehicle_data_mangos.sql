-- From zergtmn
/*
    Havenshire Stallion
    Havenshire Mare
    Havenshire Colt
*/
UPDATE creature_template SET
    spell1 = 52264,
    spell2 = 52268,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 123
WHERE entry IN (28605, 28606, 28607);

DELETE FROM npc_spellclick_spells WHERE npc_entry IN (28605, 28606, 28607);
INSERT INTO npc_spellclick_spells VALUES
(28605, 52263, 12680, 1, 12680, 1),
(28606, 52263, 12680, 1, 12680, 1),
(28607, 52263, 12680, 1, 12680, 1);
INSERT IGNORE INTO spell_script_target VALUES (52264, 1, 28653);

-- From jahangames
-- Massacre at Light's point quest
UPDATE creature_template SET
spell1 = 52435,
spell2 = 52576,
spell3 = 0,
spell4 = 0,
spell5 = 52588,
spell6 = 0,
VehicleId = 79
WHERE entry IN (28833);

UPDATE creature_template SET
spell1 = 52435,
spell2 = 52576,
spell3 = 0,
spell4 = 0,
spell5 = 52588,
spell6 = 0,
VehicleId = 68
WHERE entry IN (28887);

INSERT INTO npc_spellclick_spells VALUES ('28833', '52447', '12701', '1', '12701', '1');
INSERT INTO npc_spellclick_spells VALUES ('28887', '52447', '12701', '1', '12701', '1');
UPDATE creature_template SET minhealth = 26140, maxhealth = 26140, dynamicflags = 0, minmana = 2117, maxmana = 2117, unit_flags = 772, minlevel = 55, maxlevel = 55, unk16 = 10, unk17 = 1, InhabitType = 3, scale = 1, mindmg = 685, maxdmg = 715, armor = 3232, attackpower = 214, unit_class = 2, type = 10 WHERE entry = 28833;
UPDATE creature_template SET minhealth = 26140, maxhealth = 26140, dynamicflags = 0, minmana = 0, maxmana = 0, unit_flags = 772, minlevel = 55, maxlevel = 55, unk16 = 10, unk17 = 1, InhabitType = 3, scale = 1, mindmg = 685, maxdmg = 715, armor = 3232, attackpower = 214, unit_class = 2, type = 10 WHERE entry = 28887;
INSERT IGNORE INTO spell_script_target VALUES (52576, 1, 28834);
INSERT IGNORE INTO spell_script_target VALUES (52576, 1, 28886);
INSERT IGNORE INTO spell_script_target VALUES (52576, 1, 28850);

-- From Lanc
-- quest 12953
UPDATE `creature_template` SET
    spell1 = 55812,
    spell2 = 0,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 213
WHERE entry IN (30066);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (30066);
INSERT INTO `npc_spellclick_spells` VALUES
(30066, 44002, 12953, 1, 12953, 1);
INSERT IGNORE INTO `spell_script_target` VALUES (55812, 1, 30096);

-- From lanc
/* 7th Legion Chain Gun */
UPDATE creature_template SET
    IconName = 'Gunner',
    spell1 = 49190,
    spell2 = 49550,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 68
WHERE entry IN (27714);

DELETE FROM npc_spellclick_spells WHERE npc_entry IN (27714);
INSERT INTO npc_spellclick_spells VALUES
(27714, 67373, 0, 0, 0, 1);

/* Broken-down Shredder */
UPDATE creature_template SET
    IconName = 'vehichleCursor',
    spell1 = 48558,
    spell2 = 48604,
    spell3 = 48548,
    spell4 = 0,
    spell5 = 48610,
    spell6 = 0,
    VehicleId = 49
WHERE entry IN (27354);

DELETE FROM npc_spellclick_spells WHERE npc_entry IN (27354);
INSERT INTO npc_spellclick_spells VALUES
(27354, 67373, 0, 0, 0, 1);
INSERT IGNORE INTO spell_script_target VALUES (48610, 1, 27396);

/* Forsaken Blight Spreader */
UPDATE creature_template SET
    IconName = 'vehichleCursor',
    spell1 = 48211,
    spell2 = 0,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 36
WHERE entry IN (26523);

DELETE FROM npc_spellclick_spells WHERE npc_entry IN (26523);
INSERT INTO npc_spellclick_spells VALUES
(26523, 47961, 0, 0, 0, 1);

/* Argent Tournament mount */
UPDATE creature_template SET
    spell1 = 62544,
    spell2 = 62575,
    spell3 = 63010,
    spell4 = 62552,
    spell5 = 64077,
    spell6 = 62863,
    VehicleId = 349
WHERE entry IN (33844, 33845);
UPDATE creature_template SET KillCredit1 = 33340 WHERE entry IN (33272);
UPDATE creature_template SET KillCredit1 = 33339 WHERE entry IN (33243);

DELETE FROM npc_spellclick_spells WHERE npc_entry IN (33842, 33843);
INSERT INTO npc_spellclick_spells VALUES
(33842, 63791, 13829, 1, 0, 3),
(33842, 63791, 13839, 1, 0, 3),
(33842, 63791, 13838, 1, 0, 3),
(33843, 63792, 13828, 1, 0, 3),
(33843, 63792, 13837, 1, 0, 3),
(33843, 63792, 13835, 1, 0, 3);

DELETE FROM creature WHERE id IN (33844,33845);
UPDATE creature_template SET speed_run = '1.5', unit_flags = 8 WHERE entry IN (33844,33845);

-- Quest vehicles Support: Going Bearback (12851)
UPDATE `creature_template` SET
    spell1 = 54897,
    spell2 = 54907,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 308
WHERE entry IN (29598);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (29598);
INSERT INTO `npc_spellclick_spells` VALUES
(29598, 54908, 12851, 1, 12851, 1);

INSERT IGNORE INTO `spell_script_target` VALUES (54897, 1, 29358);

/* Scourge Gryphon */
UPDATE creature_template SET
    spell1 = 0,
    spell2 = 0,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 146
WHERE entry IN (28864);

/* Frostbrood Vanquisher */
UPDATE creature_template SET
    spell1 = 53114,
    spell2 = 53110,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 156
WHERE entry IN (28670);

UPDATE creature_template SET maxhealth = 133525, minhealth = 133525, maxmana = 51360, minmana = 51360, InhabitType = 3 WHERE entry = 28670;

REPLACE INTO `creature_template_addon` (`entry`, `mount`, `bytes1`, `b2_0_sheath`, `emote`, `moveflags`, `auras`) VALUES
(28670, 0, 50331648, 1, 0, 1024, '53112 0 53112 1 53112 2');

-- from me
-- into realm of shadows
UPDATE `creature_template` SET `IconName` = 'vehichleCursor',
`unit_flags` = 0,
`spell1` = 52362
WHERE `entry` =28782;

UPDATE `quest_template` SET 
`SrcSpell` = 52359,
`SpecialFlags` = 2,
`ReqCreatureOrGOId1` = 28768,
`ReqCreatureOrGOCount1` = 1,
`ReqSpellCast1` = 0,
`RewItemId1` = 39208,
`RewItemCount1` = 1 WHERE `entry` = 12687;

DELETE FROM `creature_involvedrelation` WHERE `quest` in (12687);
INSERT INTO `creature_involvedrelation` (`id`, `quest`) VALUES (28788, 12687);
UPDATE `creature_template` SET `npcflag` = 2 WHERE `entry` = 28788;

DELETE FROM `spell_script_target` WHERE `entry` = 52349;

UPDATE `creature_ai_scripts` SET 
`action1_type`   = '11',
`action1_param1` = '52361',
`action1_param2` = '6',
`action1_param3` = '16',
`action2_type`   = '11',
`action2_param1` = '52357',
`action2_param2` = '6',
`action2_param3` = '16',
`action3_type`   = '0'
WHERE `id` = 2876806;

DELETE FROM `creature` WHERE `id` = 28782;

DELETE FROM `creature_template_addon` WHERE `entry` = 28782;

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (28782);
INSERT INTO `npc_spellclick_spells` VALUES
(28782, 46598, 0, 0, 0, 1);

-- from lanc
-- Infected Kodo fix quest (11690)
UPDATE `creature_template` SET
spell1 = 45877,
spell2 = 0,
spell3 = 0,
spell4 = 0,
spell5 = 0,
spell6 = 0,
VehicleId = 29
WHERE `entry` IN (25596);

INSERT IGNORE INTO `spell_script_target` VALUES (45877, 1, 25596);

-- Horde Siege Tank
UPDATE `creature_template` SET
spell1 = 50672,
spell2 = 45750,
spell3 = 50677,
spell4 = 47849,
spell5 = 47962,
spell6 = 0,
VehicleId = 26
WHERE `entry` IN (25334);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (25334, 27107);
INSERT INTO `npc_spellclick_spells` VALUES
(25334, 47917, 11652, 1, 11652, 1);

REPLACE INTO `spell_script_target` VALUES (47962, 1, 27107);

REPLACE INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_start_active`, `quest_end`, `aura_spell`, `racemask`, `gender`, `autocast`) 
VALUES ('47917','4027','11652','1','11652','0','0','2','0'), ('47917','4130','11652','1','11652','0','0','2','0');

-- from lanc
-- Refurbished Shredder (quest 12050)
UPDATE `creature_template` SET
spell1 = 47939,
spell2 = 47921,
spell3 = 47966,
spell4 = 47938,
spell5 = 0,
spell6 = 0,
VehicleId = 300
WHERE `entry` IN (27061);

DELETE FROM `npc_spellclick_spells` WHERE npc_entry IN (27061);
INSERT INTO `npc_spellclick_spells` VALUES (27061, 47920, 0, 0, 0, 1);
REPLACE INTO `spell_script_target` VALUES (47939, 2, 188539);

-- Argent Cannon (quest 13086)
UPDATE `creature_template` SET
    spell1 = 57485,
    spell2 = 57412,
    spell3 = 0,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 244
WHERE `entry` IN (30236);

DELETE FROM `npc_spellclick_spells` WHERE npc_entry IN (30236);
INSERT INTO `npc_spellclick_spells` VALUES
(30236, 57573, 13086, 1, 13086, 1);

-- Wyrmrest Vanquisher (quest 12498)
UPDATE `creature_template` SET
    spell1 = 55987,
    spell2 = 50348,
    spell3 = 50430,
    spell4 = 0,
    spell5 = 0,
    spell6 = 0,
    VehicleId = 99
WHERE `entry` IN (27996);

DELETE FROM `npc_spellclick_spells` WHERE npc_entry IN (27996);
INSERT INTO `npc_spellclick_spells` VALUES
(27996, 50343, 12498, 1, 12498, 1);

REPLACE INTO `creature_template_addon` (entry, auras) VALUES (27996, '53112 0 53112 1 53112 2');

-- from me
-- Quest Reclamation (12546)
UPDATE `creature_template` SET `spell1` = 50978,`spell2` = 50980,`spell3` = 50983,`spell4` = 50985,
`VehicleId` = 111
WHERE  `entry` = 28222;

-- from YTDB/TC 578
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (27850,27881,28094,28312,28319,28670,32627,32629);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `quest_start`, `quest_start_active`, `quest_end`, `cast_flags`) VALUES
(27850, 60968, 0, 0, 0, 1),
(27881, 60968, 0, 0, 0, 1),
(28094, 60968, 0, 0, 0, 1),
(28312, 60968, 0, 0, 0, 1),
(28319, 60968, 0, 0, 0, 1),
(28670, 52196, 0, 0, 0, 1),
(32627, 60968, 0, 0, 0, 1),
(32629, 60968, 0, 0, 0, 1);

-- Quest 12996
UPDATE `creature_template` SET `spell1` = 54459,`spell2` = 54458,`spell3` = 54460,`VehicleId` = 208 WHERE  `creature_template`.`entry` = 29918;
