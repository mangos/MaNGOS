## Event 109 - Under Attack PvP Arena Gurubashi
## ======================
## Название: Under Attack PvP Arena Gurubashi
## Версия: 1.0k
## Авторы: GrEM fM Team
## Адаптировано под 3.3.5 (Кот ДаWINчи)
## ======================
# Предыстория: (В изложении Кота ДаWINчи)
#    В давние времена жил в Азероте талантливый воин. Он объездил все страны и континенты, 
# борясь со злом и  несправедливостью. За время своих  походов, он не  только отточил своё 
# мастерство и силу, но и повидал много всего ужасного. Он бился с различными порождениями 
# зла, и постепенно характер  его менялся. В  итоге он сам стал  монстром. Устав  от всего, 
# воин поселился подальше от людей, в темных подвалах развалин старой арены Гурубаши... 
#    Но время не стоит на  месте,  арену вновь  отстроили, и новые поколения бойцов  стали 
# принимать  участие  в  боях  на арене.  Шум,  говор и лязг  оружия не дают покоя старому 
# воину-монстру.  Он взбешен.  Он хочет покоя. И для этого решил сорвать бои на арене. Раз 
# в неделю в самый  разгар боев монстр появляется на арене и пытается уничтожить всех, кто
# попадется ему под руку.
## ======================
# Описание:
# На арене раз в неделю на 1,5 часа появляется монстр. Игроки должны убить его. Из убитого 
# выпадает ценная вещь.  Предъявив которую вендору у ворот арены, можно  получить элитного
# маунта. 
## ======================
# Диапазоны guid в базе:
# gameobject          - 500010 - 500017
# gameobject_template - 500000
# creature_template   - 500010, 500011
# creature            - 500010, 500011
# item_template       - 22485

# _Event_ -------------------------------------------------------------------------------
INSERT INTO `game_event` 
(`entry`, `start_time`,          `end_time`,            `occurence`, `length`, `Holiday`, `description`) VALUES 
('109',    '2009-03-22 18:00:30', '2019-02-24 06:00:00', '10080',     '150',     '0',      'Under Attack PvP Arena Gurubashi');

# Teleport ------------------------------------------------------------------------------
INSERT INTO `gameobject_template` 
(`entry`, `type`, `displayId`, `name`,                          `castBarCaption`, `faction`, `flags`, `size`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `ScriptName`) VALUES 
( 500000,  1,     6955,       'Portal to PVP Arena Gurubashi', '',                0,          0,       1,      0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,       '');
INSERT INTO `gameobject_scripts` 
(`id`, `delay`,`command`,`datalong`,`x`,     `y`,   `z`,   `o`) VALUE
(500010,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133),
(500011,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133),
(500012,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133),
(500013,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133),
(500014,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133),
(500015,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133),
(500016,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133),
(500017,0,      6,        0,        -13232.8, 218.2, 31.82, 1.133);
INSERT INTO `gameobject` VALUES 
('500010', '500000', '1', '1', '1', '-1277.71', '119.821', '131.183', '5.28871', '0', '0', '0.477', '-0.878903', '25', '0', '1'),
('500011', '500000', '0', '1', '1', '1804.87', '247.557', '60.587', '0.0268662', '0', '0', '0.0134327', '0.99991', '25', '0', '1'),
('500012', '500000', '530', '1', '1', '9502.29', '-7296.7', '14.0802', '6.16511', '0', '0', '0.0590033', '-0.998258', '25', '0', '1'),
('500013', '500000', '1', '1', '1', '9952.24', '2292.66', '1341.39', '1.56446', '0', '0', '0.704862', '0.709344', '25', '0', '1'),
('500014', '500000', '530', '1', '1', '-4003.39', '-11874.8', '-0.765862', '4.13906', '0', '0', '0.87819', '-0.478312', '25', '0', '1'),
('500015', '500000', '0', '1', '1', '-4979.75', '-884.672', '501.646', '5.39676', '0', '0', '0.428842', '-0.90338', '25', '0', '1'),
('500016', '500000', '1', '1', '1', '1535.18', '-4412.6', '11.3752', '3.24601', '0', '0', '0.998637', '-0.052185', '25', '0', '1'),
('500017', '500000', '0', '1', '1', '-8994.24', '489.263', '96.6113', '3.81739', '0', '0', '0.943454', '-0.331503', '25', '0', '1');
INSERT INTO `game_event_gameobject` VALUES 
('500010', '109'), 
('500011', '109'), 
('500012', '109'), 
('500013', '109'), 
('500014', '109'), 
('500015', '109'), 
('500016', '109'), 
('500017', '109');

# Monstr --------------------------------------------------------------------------------
INSERT INTO `creature_template` 
(`entry`, `modelid_1`, `modelid_2`, `modelid_3`, `modelid_4`, `name`,       `subname`, `IconName`, `minlevel`, `maxlevel`, `minhealth`, `maxhealth`, `minmana`, `maxmana`, `armor`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`,`scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `baseattacktime`, `rangeattacktime`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `PetSpellDataId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `RacialLeader`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`) VALUES 
( 500010,  21254,       0,           21254,       0,          'Under Boss', 'Attacker','',          83,         83,         10000000,    10000000,    5000000,   5000000,   15000,   14,          14,          0,         0.7,          1.1,        1.5,     3,      9000,     12000,    0,           9000,          0,                0,                 0,            0,              0,        0,              0,               0,               0,              0,             0,             0,                   0,      8,            500010,   0,                0,          0,             0,             0,             0,             0,             0,             0,        0,        0,        0,        0,                1000,      100000,   '',        0,              3,             0,              1,             0,              549526032,              0,            '');
#DELETE FROM `creature_loot_template` WHERE `entry` = 500010;
INSERT INTO `creature_loot_template` VALUES (500010, 22485, 100.5, 0, 1, 1, 0, 0, 0);
INSERT INTO `creature` 
(`guid`,   `id`,     `map`, `spawnMask`,`phaseMask`,`modelId`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`spawndist`,`currentwaypoint`,`curhealth`,`curmana`, `DeathState`,`MovementType`)VALUES 
('500010', '500010', '0',   '1',        '1',        '0',      '0',          '-13205.5',   '272.942',   '21.858',    '4.2155',     '150',          '5',        '0',              '10000000', '5000000', '0',         '1');
INSERT INTO `game_event_creature` VALUES ('500010', '109');

# QuestGiver ----------------------------------------------------------------------------
INSERT INTO `creature_template` 
(`entry`, `modelid_1`, `modelid_2`, `modelid_3`, `modelid_4`, `name`,     `subname`,                         `IconName`, `minlevel`, `maxlevel`, `minhealth`, `maxhealth`, `minmana`, `maxmana`, `armor`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`,`scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `baseattacktime`, `rangeattacktime`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `PetSpellDataId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `RacialLeader`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`) VALUES 
( 500011,  16779,       0,           16779,       0,          'Veroniya', 'Under Attack PvP Arena Gurubashi', '',         80,         80,         10000,       10000,       5000,      5000,      1000,    35,          35,          2,         1,            1.2,        1,       1,      10,       20,       0,           100,           0,                0,                 0,            0,              0,        0,              0,               0,               0,              0,             0,             0,                   0,      0,            0,        0,                0,          0,             0,             0,             0,             0,             0,             0,        0,        0,        0,        0,                0,         0,         '',       0,              3,             0,              1,             0,              0,                      0,            '');
INSERT INTO `creature_questrelation`    (`id`, `quest`) VALUES (500011, 500000);
INSERT INTO `creature_involvedrelation` (`id`, `quest`) VALUES (500011, 500000);
INSERT INTO `creature` VALUES ('500011', '500011', '0', '1', '1', '0', '0', '-13225.1', '235.282', '33.4367', '4.2376', '25', '0', '0', '10000', '5000', '0', '0');
INSERT INTO `game_event_creature` VALUES ('500011', '109');

REPLACE INTO game_event_creature VALUES (500011, 109);
REPLACE INTO game_event_quest VALUES (500000, 109);

# Stone ---------------------------------------------------------------------------------
INSERT INTO `item_template` 
(`entry`, `class`, `subclass`, `unk0`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `RequiredSkill`, `RequiredSkillRank`, `RequiredSpell`, `RequiredHonorRank`, `RequiredCityRank`, `RequiredReputationFaction`, `RequiredReputationRank`, `maxcount`, `stackable`, `ContainerSlots`, `StatsCount`, `stat_type1`, `stat_value1`, `stat_type2`, `stat_value2`, `stat_type3`, `stat_value3`, `stat_type4`, `stat_value4`, `stat_type5`, `stat_value5`, `stat_type6`, `stat_value6`, `stat_type7`, `stat_value7`, `stat_type8`, `stat_value8`, `stat_type9`, `stat_value9`, `stat_type10`, `stat_value10`, `ScalingStatDistribution`, `ScalingStatValue`, `dmg_min1`, `dmg_max1`, `dmg_type1`, `dmg_min2`, `dmg_max2`, `dmg_type2`,  `armor`, `holy_res`, `fire_res`, `nature_res`, `frost_res`, `shadow_res`, `arcane_res`, `delay`, `ammo_type`, `RangedModRange`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellppmRate_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`, `spellid_2`, `spelltrigger_2`, `spellcharges_2`, `spellppmRate_2`, `spellcooldown_2`, `spellcategory_2`, `spellcategorycooldown_2`, `spellid_3`, `spelltrigger_3`, `spellcharges_3`, `spellppmRate_3`, `spellcooldown_3`, `spellcategory_3`, `spellcategorycooldown_3`, `spellid_4`, `spelltrigger_4`, `spellcharges_4`, `spellppmRate_4`, `spellcooldown_4`, `spellcategory_4`, `spellcategorycooldown_4`, `spellid_5`, `spelltrigger_5`, `spellcharges_5`, `spellppmRate_5`, `spellcooldown_5`, `spellcategory_5`, `spellcategorycooldown_5`, `bonding`, `description`, `PageText`, `LanguageID`, `PageMaterial`, `startquest`, `lockid`, `Material`, `sheath`, `RandomProperty`, `RandomSuffix`, `block`, `itemset`, `MaxDurability`, `area`, `Map`, `BagFamily`, `TotemCategory`, `socketColor_1`, `socketContent_1`, `socketColor_2`, `socketContent_2`, `socketColor_3`, `socketContent_3`, `socketBonus`, `GemProperties`, `RequiredDisenchantSkill`, `ArmorDamageModifier`, `Duration`, `ItemLimitCategory`, `ScriptName`, `DisenchantID`, `FoodType`, `minMoneyLoot`, `maxMoneyLoot`) VALUES 
( 22485,   15,      0,          -1,    'Stone', 20220,       6,         0,       1,          0,          0,           0,               -1,               -1,              1,           0,               0,               0,                   0,               0,                   0,                  0,                           0,                        -1,         1,           0,                0,            0,            0,             0,            0,             0,            0,             0,            0,             0,            0,             0,            0,             0,            0,             0,            0,             0,            0,             0,             0,              0,                         0,                  0,          0,          0,           0,          0,          0,            0,       0,          0,          0,            0,           0,            0,            0,       0,           0,                0,           0,                0,                0,                -1,                0,                  -1,                       0,           0,                0,                0,                -1,                0,                 -1,                        0,           0,                0,                0,                -1,                0,                 -1,                        0,           0,                0,                0,                -1,                0,                 -1,                        0,           0,                0,                0,                -1,                0,                 -1,                        0,         '',            0,          0,            0,              0,            0,        -1,         0,        0,                0,              0,       0,         0,               0,      0,     0,           0,               0,               0,                 0,               0,                 0,               0,                 0,             0,               -1,                        0,                     0,          0,                  '',            0,              0,          0,              0);

# Quests --------------------------------------------------------------------------------
INSERT INTO `quest_template` (
`entry`, `Method`, `ZoneOrSort`, `MinLevel`, `QuestLevel`, 
`Type`, `RequiredClasses`, `RequiredRaces`, `RequiredSkill`, `RequiredSkillValue`, `RepObjectiveFaction`,
`RepObjectiveValue`, `RequiredMinRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepFaction`, `RequiredMaxRepValue`, 
`SuggestedPlayers`, `LimitTime`, `QuestFlags`, `SpecialFlags`, `CharTitleId`, 
`PlayersSlain`, `BonusTalents`, `PrevQuestId`, `NextQuestId`, `ExclusiveGroup`, 
`NextQuestInChain`, `SrcItemId`, `SrcItemCount`, `SrcSpell`, `Title`, 
`Details`, `Objectives`, `OfferRewardText`, `RequestItemsText`, `EndText`, 
`ObjectiveText1`, `ObjectiveText2`, `ObjectiveText3`, `ObjectiveText4`, `ReqItemId1`, 
`ReqItemId2`,`ReqItemId3`, `ReqItemId4`, `ReqItemCount1`, `ReqItemCount2`,
`ReqItemCount3`, `ReqItemCount4`, `ReqSourceId1`, `ReqSourceId2`, `ReqSourceId3`, 
`ReqSourceId4`, `ReqSourceCount1`, `ReqSourceCount2`, `ReqSourceCount3`, `ReqSourceCount4`, 
`ReqCreatureOrGOId1`, `ReqCreatureOrGOId2`, `ReqCreatureOrGOId3`, `ReqCreatureOrGOId4`, `ReqCreatureOrGOCount1`, 
`ReqCreatureOrGOCount2`, `ReqCreatureOrGOCount3`, `ReqCreatureOrGOCount4`, `ReqSpellCast1`, `ReqSpellCast2`, 
`ReqSpellCast3`, `ReqSpellCast4`, `RewChoiceItemId1`, `RewChoiceItemId2`, `RewChoiceItemId3`, 
`RewChoiceItemId4`, `RewChoiceItemId5`, `RewChoiceItemId6`, `RewChoiceItemCount1`, `RewChoiceItemCount2`, 
`RewChoiceItemCount3`, `RewChoiceItemCount4`, `RewChoiceItemCount5`, `RewChoiceItemCount6`, `RewItemId1`, 
`RewItemId2`, `RewItemId3`, `RewItemId4`, `RewItemCount1`, `RewItemCount2`, 
`RewItemCount3`, `RewItemCount4`, `RewRepFaction1`, `RewRepFaction2`, `RewRepFaction3`, 
`RewRepFaction4`, `RewRepFaction5`, `RewRepValue1`, `RewRepValue2`, `RewRepValue3`, 
`RewRepValue4`, `RewRepValue5`, `RewHonorAddition`, `RewOrReqMoney`, `RewMoneyMaxLevel`, 
`RewSpell`, `RewSpellCast`, `RewMailTemplateId`, `RewMailDelaySecs`, `PointMapId`, 
`PointX`, `PointY`, `PointOpt`, `DetailsEmote1`, `DetailsEmote2`, 
`DetailsEmote3`, `DetailsEmote4`, `IncompleteEmote`, `CompleteEmote`, `OfferRewardEmote1`, 
`OfferRewardEmote2`, `OfferRewardEmote3`, `OfferRewardEmote4`, `StartScript`, `CompleteScript`) VALUES 
(500000,2,     0,     80,    80, 
 0,     0,     0,     0,     0, 0,
 0,     0,     0,     0,     0, 
 0,     750,   2,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     'Under Attack PvP Arena Gurubashi', 
'Well hi, the man of courage.', 
'Kill the boss and take away from it a stone. As the award you will receive a gift from GrEM fM Team.', 
'Here hold has deserved. This event has made GrEM fM Team.', 
'Come back when you will extract a stone.', 
'Good luck!', 
 '',    '',    '',    '',    22485, 
 0,     0,     0,     1,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     44707, 44413, 44151, 
 44168, 41508, 0,     1,     1, 
 1,     1,     1,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     10000, 0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0, 
 0,     0,     0,     0,     0);

# *The_END*
