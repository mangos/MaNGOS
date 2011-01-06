## event 101 - Legion Attack Shattrat City
## ======================
## Название: Legion Attack Shattrat City
## Версия: 1.0
## Авторы: GrEM fM Team
## Адаптировано под 3.3.5 (Кот ДаWINчи)
## ======================
## Описание: 
#  Передовые отряды легиона просочились в Шаттрат  и заняли центр города. 
#  Игрокам необходимо освободить центральный зал от врагов. За 30 убитых
#  врагов вас ждет награда. 
## ======================
# Диапазоны guid в базе:
# gameobject          - 500020 - 50027
# gameobject_template - 500001
# creature_template   - 500020,500021
# creature            - 500200 - 500230,500240

# _Event_ ---------------------------------------------------------------------
INSERT INTO `game_event` 
(`entry`, `start_time`,          `end_time`,           `occurence`, `length`, `Holiday`, `description`) VALUES 
('101',   '2009-03-26 07:00:00', '2019-02-25 23:50:00', '4600',     '180',    '0',       'Legion Attack Shattrat City');

# NPC -------------------------------------------------------------------------
# Legioners
INSERT INTO `creature_template` 
(`entry`, `modelid_1`, `modelid_2`, `modelid_3`, `modelid_4`, `name`,     `subname`,            `IconName`, `minlevel`, `maxlevel`, `minhealth`, `maxhealth`, `minmana`, `maxmana`, `armor`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `baseattacktime`, `rangeattacktime`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `PetSpellDataId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `RacialLeader`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`) VALUES 
( 500020,  5231,        0,           5231,        0,          'Legioner', 'Legion the attacker', '',         81,         81,         15000,       15000,       0,         0,         10000,   14,          14,          0,         1,            1.3,         1,       0,      1000,     2600,     0,           3000,          0,                0,                 0,            0,              0,        0,              0,               0,               0,              0,             0,             0,                   0,      0,            0,        0,                0,          0,             0,             0,             0,             0,             0,             0,        0,        0,        0,        0,                1000,      10000,    '',        0,              3,             0,              1,             0,              0,                      0,            '');
INSERT INTO `creature` VALUES 
('500200', '500020', '530', '1', '1', '0', '0', '-1837.11', '5434.27', '-12.4279', '5.97908', '25', '0', '0', '15000', '0', '0', '0'),
('500201', '500020', '530', '1', '1', '0', '0', '-1844.05', '5448.26', '-12.4281', '0.0972285', '25', '0', '0', '15000', '0', '0', '0'),
('500202', '500020', '530', '1', '1', '0', '0', '-1838.55', '5442.12', '-12.4281', '0.145133', '25', '0', '0', '15000', '0', '0', '0'),
('500203', '500020', '530', '1', '1', '0', '0', '-1837.69', '5425.77', '-12.4281', '5.8701',  '25', '0', '0', '15000', '0', '0', '0'),
('500204', '500020', '530', '1', '1', '0', '0', '-1843.77', '5415.44', '-12.4281', '5.62105', '25', '0', '0', '15000', '0', '0', '0'),
('500205', '500020', '530', '1', '1', '0', '0', '-1857.53', '5407.44', '-12.4281', '5.16937', '25', '0', '0', '15000', '0', '0', '0'),
('500206', '500020', '530', '1', '1', '0', '0', '-1873.69', '5409.51', '-12.4279', '4.39713', '25', '0', '0', '15000', '0', '0', '0'),
('500207', '500020', '530', '1', '1', '0', '0', '-1882.02', '5417.84', '-12.4279', '3.52298', '25', '0', '0', '15000', '0', '0', '0'),
('500208', '500020', '530', '1', '1', '0', '0', '-1886',    '5431.77', '-12.4279', '3.13853', '25', '0', '0', '15000', '0', '0', '0'),
('500209', '500020', '530', '1', '1', '0', '0', '-1879.57', '5444.11', '-12.4279', '2.49613', '25', '0', '0', '15000', '0', '0', '0'),
('500210', '500020', '530', '1', '1', '0', '0', '-1870.93', '5450.06', '-12.4279', '2.30689', '25', '0', '0', '15000', '0', '0', '0'),
('500211', '500020', '530', '1', '1', '0', '0', '-1858.99', '5454.19', '-12.4279', '1.37934', '25', '0', '0', '15000', '0', '0', '0'),
('500212', '500020', '530', '1', '1', '0', '0', '-1865.31', '5420.17', '-10.4635', '4.57904', '25', '0', '0', '15000', '0', '0', '0'),
('500213', '500020', '530', '1', '1', '0', '0', '-1872.37', '5434.62', '-10.4635', '2.76226', '25', '0', '0', '15000', '0', '0', '0'),
('500214', '500020', '530', '1', '1', '0', '0', '-1862.21', '5440.86', '-10.4635', '1.4357',  '25', '0', '0', '15000', '0', '0', '0'),
('500215', '500020', '530', '1', '1', '0', '0', '-1853.12', '5428.8',  '-10.465',  '5.96195', '25', '0', '0', '15000', '0', '0', '0'),
('500216', '500020', '530', '1', '1', '0', '0', '-1832.85', '5367.45', '-12.4279', '2.01271', '25', '0', '0', '15000', '0', '0', '0'),
('500217', '500020', '530', '1', '1', '0', '0', '-1881.65', '5377.25', '-12.4278', '1.2325',  '25', '0', '0', '15000', '0', '0', '0'),
('500218', '500020', '530', '1', '1', '0', '0', '-1925.47', '5399.78', '-12.4269', '0.445534', '25', '0', '0', '15000', '0', '0', '0'),
('500219', '500020', '530', '1', '1', '0', '0', '-1918.07', '5448.88', '-12.4277', '5.97709', '25', '0', '0', '15000', '0', '0', '0'),
('500220', '500020', '530', '1', '1', '0', '0', '-1891.06', '5484.28', '-12.4269', '5.13684', '25', '0', '0', '15000', '0', '0', '0'),
('500221', '500020', '530', '1', '1', '0', '0', '-1844.84', '5486.86', '-12.4282', '4.42527', '25', '0', '0', '15000', '0', '0', '0'),
('500222', '500020', '530', '1', '1', '0', '0', '-1809.69', '5458.05', '-12.4283', '3.63203', '25', '0', '0', '15000', '0', '0', '0'),
('500223', '500020', '530', '1', '1', '0', '0', '-1813.89', '5436.43', '-12.4279', '3.27939', '25', '0', '0', '15000', '0', '0', '0'),
('500224', '500020', '530', '1', '1', '0', '0', '-1831.6',  '5391.66', '-12.4279', '2.35026', '25', '0', '0', '15000', '0', '0', '0'),
('500225', '500020', '530', '1', '1', '0', '0', '-1853.34', '5381.21', '-12.4279', '1.69131', '25', '0', '0', '15000', '0', '0', '0'),
('500226', '500020', '530', '1', '1', '0', '0', '-1896.37', '5392.58', '-12.4282', '0.859573', '25', '0', '0', '15000', '0', '0', '0'),
('500227', '500020', '530', '1', '1', '0', '0', '-1912.65', '5422.91', '-12.4274', '0.197482', '25', '0', '0', '15000', '0', '0', '0'),
('500228', '500020', '530', '1', '1', '0', '0', '-1902.64', '5463.61', '-12.4279', '5.58453', '25', '0', '0', '15000', '0', '0', '0'),
('500229', '500020', '530', '1', '1', '0', '0', '-1871.74', '5480.14', '-12.4276', '4.8117',  '25', '0', '0', '15000', '0', '0', '0'),
('500230', '500020', '530', '1', '1', '0', '0', '-1830.19', '5467.15', '-12.428',  '3.97761', '25', '0', '0', '15000', '0', '0', '0');
INSERT INTO `game_event_creature` VALUES 
('500200', '101'),
('500201', '101'),
('500202', '101'),
('500203', '101'),
('500204', '101'),
('500205', '101'),
('500206', '101'),
('500207', '101'),
('500208', '101'),
('500209', '101'),
('500210', '101'),
('500211', '101'),
('500212', '101'),
('500213', '101'),
('500214', '101'),
('500215', '101'),
('500216', '101'),
('500217', '101'),
('500218', '101'),
('500219', '101'),
('500220', '101'),
('500221', '101'),
('500222', '101'),
('500223', '101'),
('500224', '101'),
('500225', '101'),
('500226', '101'),
('500227', '101'),
('500228', '101'),
('500229', '101'),
('500230', '101');

# QuestGiver ------------------------------------------------------------------
INSERT INTO `creature_template` 
(`entry`, `modelid_1`, `modelid_2`, `modelid_3`, `modelid_4`, `name`,                `subname`, `IconName`, `minlevel`, `maxlevel`, `minhealth`, `maxhealth`, `minmana`, `maxmana`, `armor`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `baseattacktime`, `rangeattacktime`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `PetSpellDataId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `RacialLeader`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`) VALUES 
( 500021,  1499,        0,           1499,        0,          'Scount  Anetotalaor', 'Defender', '',         81,         81,         25000,       25000,       0,         0,         16000,   35,          35,          2,         1,            1.27,        1,       1,      1000,     2000,     0,           3000,          0,                0,                 0,            0,              0,        0,              0,               0,               0,              0,             0,             0,                   0,      0,            0,        0,                0,          0,             0,             0,             0,             0,             0,             0,        0,        0,        0,        0,                0,         0,        '',        0,              3,             0,              1,             0,              0,                      0,             '');
INSERT INTO `creature_questrelation`    (`id`, `quest`) VALUES (500021, 500001);
INSERT INTO `creature_involvedrelation` (`id`, `quest`) VALUES (500021, 500001);
INSERT INTO `creature` VALUES 
('500240', '500021', '530', '1', '1', '0', '0', '-1818.48', '5415.42', '-12.4274', '2.8477', '25', '0', '0', '25000', '0', '0', '0');
INSERT INTO `game_event_creature` VALUES ('500240', '101');

# Quest -----------------------------------------------------------------------
INSERT INTO `quest_template` 
(`entry`, `Method`, `ZoneOrSort`, `MinLevel`, `QuestLevel`, `Type`, `RequiredClasses`, `RequiredRaces`, `RequiredSkill`, `RequiredSkillValue`, `RepObjectiveFaction`, `RepObjectiveValue`, `RequiredMinRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepFaction`, `RequiredMaxRepValue`, `SuggestedPlayers`, `LimitTime`, `QuestFlags`, `SpecialFlags`, `CharTitleId`, `PlayersSlain`, `BonusTalents`, `PrevQuestId`, `NextQuestId`, `ExclusiveGroup`, `NextQuestInChain`, `SrcItemId`, `SrcItemCount`, `SrcSpell`, `Title`, `Details`, `Objectives`, `OfferRewardText`, `RequestItemsText`, `EndText`, `ObjectiveText1`, `ObjectiveText2`, `ObjectiveText3`, `ObjectiveText4`, `ReqItemId1`, `ReqItemId2`, `ReqItemId3`, `ReqItemId4`, `ReqItemCount1`, `ReqItemCount2`, `ReqItemCount3`, `ReqItemCount4`, `ReqSourceId1`, `ReqSourceId2`, `ReqSourceId3`, `ReqSourceId4`, `ReqSourceCount1`, `ReqSourceCount2`, `ReqSourceCount3`, `ReqSourceCount4`, `ReqCreatureOrGOId1`, `ReqCreatureOrGOId2`, `ReqCreatureOrGOId3`, `ReqCreatureOrGOId4`, `ReqCreatureOrGOCount1`, `ReqCreatureOrGOCount2`, `ReqCreatureOrGOCount3`, `ReqCreatureOrGOCount4`, `ReqSpellCast1`, `ReqSpellCast2`, `ReqSpellCast3`, `ReqSpellCast4`, `RewChoiceItemId1`, `RewChoiceItemId2`, `RewChoiceItemId3`, `RewChoiceItemId4`, `RewChoiceItemId5`, `RewChoiceItemId6`, `RewChoiceItemCount1`, `RewChoiceItemCount2`, `RewChoiceItemCount3`, `RewChoiceItemCount4`, `RewChoiceItemCount5`, `RewChoiceItemCount6`, `RewItemId1`, `RewItemId2`, `RewItemId3`, `RewItemId4`, `RewItemCount1`, `RewItemCount2`, `RewItemCount3`, `RewItemCount4`, `RewRepFaction1`, `RewRepFaction2`, `RewRepFaction3`, `RewRepFaction4`, `RewRepFaction5`, `RewRepValue1`, `RewRepValue2`, `RewRepValue3`, `RewRepValue4`, `RewRepValue5`, `RewHonorAddition`, `RewOrReqMoney`, `RewMoneyMaxLevel`, `RewSpell`, `RewSpellCast`, `RewMailTemplateId`, `RewMailDelaySecs`, `PointMapId`, `PointX`, `PointY`, `PointOpt`, `DetailsEmote1`, `DetailsEmote2`, `DetailsEmote3`, `DetailsEmote4`, `IncompleteEmote`, `CompleteEmote`, `OfferRewardEmote1`, `OfferRewardEmote2`, `OfferRewardEmote3`, `OfferRewardEmote4`, `StartScript`, `CompleteScript`) VALUES 
(500001, 2, 0, 80, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Legion Attack Shattrat City', 'About thank God you has survived. Help us on us the legion has attacked!!!! $BThey have attacked us when we prepared for an attack on them, but they have outstripped us. $BThese creatures have crept away on all Terrace of Light (centre Shattrath City)', 'Kill 30 legionaries, we will be very grateful to you. $BThe award it will be obligatory!', 'This event has made GrEM fM Team.', 'Come back when you will kill 30 legionaries.', 'Good luck!', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 500020, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 23162, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 21215, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
REPLACE INTO game_event_creature VALUES (500021, 101);
REPLACE INTO game_event_quest VALUES (500001, 101);

# Teleports -------------------------------------------------------------------
INSERT INTO `gameobject_template` 
(`entry`, `type`, `displayId`, `name`,                `castBarCaption`, `faction`, `flags`, `size`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `ScriptName`) VALUES 
( 500001,  22,     7146,       'Portal to Shattrath', '',                0,         0,       1,      33728,   0,       0,       0,       0,       0,       0,       0,       0,       0,       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,       '');
INSERT INTO `gameobject` VALUES 
('500020', '500001', '0',   '1', '1', '-8987.36', '495.888',  '96.5049',   '3.80482',   '0', '0', '0.945519',  '-0.325567',  '25', '0', '1'),
('500021', '500001', '1',   '1', '1', '1528.88',  '-4413.28', '12.9977',   '3.19614',   '0', '0', '0.999628',  '-0.0272726', '25', '0', '1'),
('500022', '500001', '1',   '1', '1', '-1277.71', '119.821',  '131.183',   '5.28871',   '0', '0', '0.477',     '-0.878903',  '25', '0', '1'),
('500023', '500001', '0',   '1', '1', '1804.87',  '247.557',  '60.587',    '0.0268662', '0', '0', '0.0134327', '0.99991',    '25', '0', '1'),
('500024', '500001', '530', '1', '1', '9502.29',  '-7296.7',  '14.0802',   '6.16511',   '0', '0', '0.0590033', '-0.998258',  '25', '0', '1'),
('500025', '500001', '1',   '1', '1', '9952.24',  '2292.66',  '1341.39',   '1.56446',   '0', '0', '0.704862',  '0.709344',   '25', '0', '1'),
('500026', '500001', '530', '1', '1', '-4003.39', '-11874.8', '-0.765862', '4.13906',   '0', '0', '0.87819',   '-0.478312',  '25', '0', '1'),
('500027', '500001', '0',   '1', '1', '-4979.75', '-884.672', '501.646',   '5.39676',   '0', '0', '0.428842',  '-0.90338',   '25', '0', '1');
INSERT INTO `game_event_gameobject` VALUES 
('500020', '101'),
('500021', '101'),
('500022', '101'),
('500023', '101'),
('500024', '101'),
('500025', '101'),
('500026', '101'),
('500027', '101');

# The_END!
