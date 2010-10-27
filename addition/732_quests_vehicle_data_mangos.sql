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

-- from me
-- into realm of shadows
#UPDATE `creature_template` SET `unit_flags` = '16777224' WHERE `entry` =28782;
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
`RewItemId1` = 0,
`RewItemCount1` = 0 WHERE `entry` = 12687;

DELETE FROM `creature_involvedrelation` WHERE `quest` in (12687);
INSERT INTO `creature_involvedrelation` (`id`, `quest`) VALUES (28788, 12687);

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
