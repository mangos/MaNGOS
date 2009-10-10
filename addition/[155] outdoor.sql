-- del GOs
DELETE FROM gameobject_respawn WHERE guid IN (SELECT guid FROM gameobject WHERE id IN (182267, 182280,182281,182282,182222,182272,182273,182274,182266,182275,182276,182277,182301,182302,182303,182304,182305,182306,182307,182308,182297,182298,182299,182300));
DELETE FROM gameobject WHERE id IN (182267, 182280,182281,182282,182222,182272,182273,182274,182266,182275,182276,182277,182301,182302,182303,182304,182305,182306,182307,182308,182297,182298,182299,182300);

-- del NPCs
DELETE FROM creature_addon WHERE guid IN (select guid from creature WHERE id IN (18816,18821,21474,21484,21483,18192,18817,18822,21485,21487,21488,18256));
DELETE FROM creature_respawn WHERE guid IN (select guid from creature WHERE id IN (18816,18821,21474,21484,21483,18192,18817,18822,21485,21487,21488,18256));
DELETE FROM creature WHERE id IN (18816,18821,21474,21484,21483,18192,18817,18822,21485,21487,21488,18256);

-- creature template changes
-- guards don't regen health
UPDATE creature_template SET RegenHealth = 0 WHERE entry IN (18192,18256);
--
UPDATE creature_template SET faction_A = 1630, faction_H = 1630, ScriptName = 'mob_eventai' WHERE entry = 18225;

-- pvp flag
UPDATE creature_template SET unit_flags = unit_flags | 0x1000 WHERE entry IN (18816,18821,21474,21484,21483,18192,18817,18822,21485,21487,21488,18256);

-- faction changes
-- (destroyed) wyvern posts
UPDATE gameobject_template SET faction = 84, flags = 0 WHERE entry IN (182266,182267,182275,182276,182277,182280,182281,182282);
UPDATE gameobject_template SET faction = 83, flags = 0 WHERE entry IN (182297, 182298, 182299, 182300, 182301, 182302, 182303, 182304);
-- bomb wagons
UPDATE gameobject_template SET faction = 83, flags = 0 WHERE entry IN (182222, 182272, 182273, 182274); -- horde friendly
UPDATE gameobject_template SET faction = 84, flags = 0 WHERE entry IN (182305, 182306, 182307, 182308); -- ally friendly

UPDATE gameobject_template SET data0 = 0 WHERE entry IN (181598, 181597); -- without this, the client sends a spellcast packet on go use instead of use go packet
UPDATE gameobject_template SET data0 = 0 WHERE entry = 181682;

DELETE from gameobject WHERE id = 181682; -- DELETE the go that is spawned by the opvp
DELETE from creature WHERE id = 17209; -- DELETE the flight master that is spawned by the opvp

UPDATE creature_template SET faction_a = 83, faction_h = 83 WHERE entry = 17995; -- horde guards faction
UPDATE creature_template SET faction_a = 83, faction_h = 83 WHERE entry = 17996;
UPDATE creature_template SET faction_a = 84, faction_h = 84 WHERE entry = 17635; -- ally guards faction
UPDATE creature_template SET faction_a = 84, faction_h = 84 WHERE entry = 17647;
UPDATE creature_template SET faction_a = 84, faction_h = 83 WHERE entry = 17209; -- flight master
UPDATE creature_template SET AIName = 'OutdoorPvPObjectiveAI', ScriptName = '' WHERE entry = 12999;
UPDATE creature_template SET npcflag = npcflag | 536870912 WHERE entry IN (18564, 18581);

DELETE FROM game_graveyard_zone WHERE id IN (969, 927, 993); -- DELETE na, za, epl pvp graveyard associations, they should only be available to the controlling faction

UPDATE quest_template SET ReqSpellCast1 = 0, ReqSpellCast2 = 0, ReqSpellCast3 = 0, ReqSpellCast4 = 0 WHERE entry IN (9665, 9664); -- fix quests IN epl
UPDATE quest_template SET reqspellcast1 = 0, reqspellcast2 = 0, reqspellcast3 = 0, reqspellcast4 = 0 WHERE entry in(9664,9665);

DELETE FROM gameobject WHERE id IN (181899, 182096, 182097, 182173, 182174, 182175, 182210, 182522, 182523, 183104, 183411, 183412, 183413, 183414, 182098);
DELETE FROM creature WHERE id IN (18564, 18581);

DELETE FROM npc_option WHERE id = 51;
INSERT INTO npc_option(id, gossip_id, npcflag, icon, action, option_text) VALUES (51,0,536870912,0,18,"UNIT_NPC_FLAG_OUTDOORPVP");


DELETE FROM `mangos_string` WHERE `entry` BETWEEN 9001 AND 9050;
INSERT INTO `mangos_string` (`entry`, `content_default`) VALUES
-- hellfire peninsula
(9001,'The Horde has taken The Overlook!'),
(9002,'The Alliance has taken The Overlook!'),
(9003,'The Horde has taken The Stadium!'),
(9004,'The Alliance has taken The Stadium!'),
(9005,'The Horde has taken Broken Hill!'),
(9006,'The Alliance has taken Broken Hill!'),
(9007,'The Horde lost The Overlook!'),
(9008,'The Alliance lost The Overlook!'),
(9009,'The Horde lost The Stadium!'),
(9010,'The Alliance lost The Stadium!'),
(9011,'The Horde lost Broken Hill!'),
(9012,'The Alliance lost Broken Hill!'),
-- zangarmarsh
(9013,'The Horde has taken the West Beacon!'),
(9014,'The Alliance has taken the West Beacon!'),
(9015,'The Horde has taken the East Beacon!'),
(9016,'The Alliance has taken the East Beacon!'),
(9017,'The Horde has captured the Twin Spire Graveyard!'),
(9018,'The Alliance has captured the Twin Spire Graveyard!'),
(9019,'The Horde lost the West Beacon!'),
(9020,'The Alliance lost the West Beacon!'),
(9021,'The Horde lost the East Beacon!'),
(9022,'The Alliance lost the East Beacon!'),
(9023,'The Horde lost the Twin Spire Graveyard!'),
(9024,'The Alliance lost the Twin Spire Graveyard!'),
-- nagrand
(9025,'The Horde has captured Halaa!'),
(9026,'The Alliance has captured Halaa!'),
(9027,'The Horde lost Halaa!'),
(9028,'The Alliance lost Halaa!'),
-- terokkar forest
(9029,'The Horde has taken a Spirit Tower!'),
(9030,'The Alliance has taken a Spirit Tower!'),
(9031,'The Horde lost a Spirit Tower!'),
(9032,'The Alliance lost a Spirit Tower!'),
-- eastern plaguelands
(9033,'The Horde has taken the Northpass Tower!'),
(9034,'The Alliance has taken the Northpass Tower!'),
(9035,'The Horde has taken the Eastwall Tower!'),
(9036,'The Alliance has taken the Eastwall Tower!'),
(9037,'The Horde has taken the Crown Guard Tower!'),
(9038,'The Alliance has taken the Crown Guard Tower!'),
(9039,'The Horde has taken the Plaguewood Tower!'),
(9040,'The Alliance has taken the Plaguewood Tower!'),
(9041,'The Horde lost the Northpass Tower!'),
(9042,'The Alliance lost the Northpass Tower!'),
(9043,'The Horde lost the Eastwall Tower!'),
(9044,'The Alliance lost the Eastwall Tower!'),
(9045,'The Horde lost the Crown Guard Tower!'),
(9046,'The Alliance lost the Crown Guard Tower!'),
(9047,'The Horde lost the Plaguewood Tower!'),
(9048,'The Alliance lost the Plaguewood Tower!'),
-- silithus
(9049,'The Horde has collected 200 silithyst!'),
(9050,'The Alliance has collected 200 silithyst!');
