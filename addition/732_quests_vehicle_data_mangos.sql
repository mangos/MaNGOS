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
-- Into the realm of Shadows quest
UPDATE creature_template SET
spell1 = 52362,
spell2 = 0,
spell3 = 0,
spell4 = 0,
spell5 = 0,
spell6 = 0,
VehicleId = 135
WHERE entry IN (28782);

DELETE FROM npc_spellclick_spells WHERE npc_entry in (28782);
INSERT INTO npc_spellclick_spells VALUES (28782, 52349, 12687, 1, 12687, 1);
INSERT IGNORE INTO spell_script_target VALUES (52349, 1, 28782);
DELETE FROM creature_involvedrelation WHERE quest in (12687);
INSERT INTO creature_involvedrelation (id, quest) VALUES (28788, 12687);
UPDATE creature_template SET npcflag=npcflag|2 WHERE entry=28788;
UPDATE quest_template SET SpecialFlags = 0 , SuggestedPlayers = 0 , Method = 1 WHERE entry IN (12687);

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
