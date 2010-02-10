ALTER TABLE db_version CHANGE COLUMN required_8608_01_mangos_mangos_string required_8608_02_mangos_battleground_events bit;

DELETE FROM battleground_events WHERE map = 30;
INSERT INTO battleground_events (map, event1, event2, description) VALUES
-- alterac valley
    (30, 254, 0, 'Doors'),

    (30, 0, 0, 'Firstaid Station - Alliance assaulted'),
    (30, 0, 1, 'Firstaid Station - ALliance control'),
    (30, 0, 2, 'Firstaid Station - Horde assaulted'),
    (30, 0, 3, 'Firstaid Station - Horde control'),

    (30, 1, 0, 'Stormpike Grave - Alliance assaulted'),
    (30, 1, 1, 'Stormpike Grave - ALliance control'),
    (30, 1, 2, 'Stormpike Grave - Horde assaulted'),
    (30, 1, 3, 'Stormpike Grave - Horde control'),

    (30, 2, 0, 'Stoneheart Grave - Alliance assaulted'),
    (30, 2, 1, 'Stoneheart Grave - ALliance control'),
    (30, 2, 2, 'Stoneheart Grave - Horde assaulted'),
    (30, 2, 3, 'Stoneheart Grave - Horde control'),

    (30, 3, 0, 'Snowfall Grave - Alliance assaulted'),
    (30, 3, 1, 'Snowfall Grave - ALliance control'),
    (30, 3, 2, 'Snowfall Grave - Horde assaulted'),
    (30, 3, 3, 'Snowfall Grave - Horde control'),
    (30, 3, 5, 'Snowfall Grave - Neutral control'),

    (30, 4, 0, 'Iceblood Grave - Alliance assaulted'),
    (30, 4, 1, 'Iceblood Grave - ALliance control'),
    (30, 4, 2, 'Iceblood Grave - Horde assaulted'),
    (30, 4, 3, 'Iceblood Grave - Horde control'),

    (30, 5, 0, 'Frostwolf Grave - Alliance assaulted'),
    (30, 5, 1, 'Frostwolf Grave - ALliance control'),
    (30, 5, 2, 'Frostwolf Grave - Horde assaulted'),
    (30, 5, 3, 'Frostwolf Grave - Horde control'),

    (30, 6, 0, 'Frostwolf Hut - Alliance assaulted'),
    (30, 6, 1, 'Frostwolf Hut - ALliance control'),
    (30, 6, 2, 'Frostwolf Hut - Horde assaulted'),
    (30, 6, 3, 'Frostwolf Hut - Horde control'),

    -- (30, 7, 0, 'Dunbaldar South - Alliance assaulted'),
    (30, 7, 1, 'Dunbaldar South - ALliance control'),
    (30, 7, 2, 'Dunbaldar South - Horde assaulted'),
    (30, 7, 3, 'Dunbaldar South - Horde control'),

    -- (30, 8, 0, 'Dunbaldar North - Alliance assaulted'),
    (30, 8, 1, 'Dunbaldar North - ALliance control'),
    (30, 8, 2, 'Dunbaldar North - Horde assaulted'),
    (30, 8, 3, 'Dunbaldar North - Horde control'),

    -- (30, 9, 0, 'Icewing Bunker - Alliance assaulted'),
    (30, 9, 1, 'Icewing Bunker - ALliance control'),
    (30, 9, 2, 'Icewing Bunker - Horde assaulted'),
    (30, 9, 3, 'Icewing Bunker - Horde control'),

    -- (30, 10, 0, 'Stoneheart Bunker - Alliance assaulted'),
    (30, 10, 1, 'Stoneheart Bunker - ALliance control'),
    (30, 10, 2, 'Stoneheart Bunker - Horde assaulted'),
    (30, 10, 3, 'Stoneheart Bunker - Horde control'),

    (30, 11, 0, 'Iceblood Tower - Alliance assaulted'),
    (30, 11, 1, 'Iceblood Tower - ALliance control'),
    -- (30, 11, 2, 'Iceblood Tower - Horde assaulted'),
    (30, 11, 3, 'Iceblood Tower - Horde control'),

    (30, 12, 0, 'Tower Point - Alliance assaulted'),
    (30, 12, 1, 'Tower Point - ALliance control'),
    -- (30, 12, 2, 'Tower Point - Horde assaulted'),
    (30, 12, 3, 'Tower Point - Horde control'),

    (30, 13, 0, 'Frostwolf east Tower - Alliance assaulted'),
    (30, 13, 1, 'Frostwolf east Tower - ALliance control'),
    -- (30, 13, 2, 'Frostwolf east Tower - Horde assaulted'),
    (30, 13, 3, 'Frostwolf east Tower - Horde control'),

    (30, 14, 0, 'Frostwolf west Tower - Alliance assaulted'),
    (30, 14, 1, 'Frostwolf west Tower - ALliance control'),
    -- (30, 14, 2, 'Frostwolf west Tower - Horde assaulted'),
    (30, 14, 3, 'Frostwolf west Tower - Horde control'),


    (30, 15, 0, 'Firstaid Station - Alliance Defender Quest0'),
    (30, 15, 1, 'Firstaid Station - Alliance Defender Quest1'),
    (30, 15, 2, 'Firstaid Station - Alliance Defender Quest2'),
    (30, 15, 3, 'Firstaid Station - Alliance Defender Quest3'),
    (30, 15, 4, 'Firstaid Station - Horde Defender Quest0'),
    (30, 15, 5, 'Firstaid Station - Horde Defender Quest1'),
    (30, 15, 6, 'Firstaid Station - Horde Defender Quest2'),
    (30, 15, 7, 'Firstaid Station - Horde Defender Quest3'),

    (30, 16, 0, 'Stormpike Grave - Alliance Defender Quest0'),
    (30, 16, 1, 'Stormpike Grave - Alliance Defender Quest1'),
    (30, 16, 2, 'Stormpike Grave - Alliance Defender Quest2'),
    (30, 16, 3, 'Stormpike Grave - Alliance Defender Quest3'),
    (30, 16, 4, 'Stormpike Grave - Horde Defender Quest0'),
    (30, 16, 5, 'Stormpike Grave - Horde Defender Quest1'),
    (30, 16, 6, 'Stormpike Grave - Horde Defender Quest2'),
    (30, 16, 7, 'Stormpike Grave - Horde Defender Quest3'),

    (30, 17, 0, 'Stoneheart Grave - Alliance Defender Quest0'),
    (30, 17, 1, 'Stoneheart Grave - Alliance Defender Quest1'),
    (30, 17, 2, 'Stoneheart Grave - Alliance Defender Quest2'),
    (30, 17, 3, 'Stoneheart Grave - Alliance Defender Quest3'),
    (30, 17, 4, 'Stoneheart Grave - Horde Defender Quest0'),
    (30, 17, 5, 'Stoneheart Grave - Horde Defender Quest1'),
    (30, 17, 6, 'Stoneheart Grave - Horde Defender Quest2'),
    (30, 17, 7, 'Stoneheart Grave - Horde Defender Quest3'),

    (30, 18, 0, 'Snowfall Grave - Alliance Defender Quest0'),
    (30, 18, 1, 'Snowfall Grave - Alliance Defender Quest1'),
    (30, 18, 2, 'Snowfall Grave - Alliance Defender Quest2'),
    (30, 18, 3, 'Snowfall Grave - Alliance Defender Quest3'),
    (30, 18, 4, 'Snowfall Grave - Horde Defender Quest0'),
    (30, 18, 5, 'Snowfall Grave - Horde Defender Quest1'),
    (30, 18, 6, 'Snowfall Grave - Horde Defender Quest2'),
    (30, 18, 7, 'Snowfall Grave - Horde Defender Quest3'),

    (30, 19, 0, 'Iceblood Grave - Alliance Defender Quest0'),
    (30, 19, 1, 'Iceblood Grave - Alliance Defender Quest1'),
    (30, 19, 2, 'Iceblood Grave - Alliance Defender Quest2'),
    (30, 19, 3, 'Iceblood Grave - Alliance Defender Quest3'),
    (30, 19, 4, 'Iceblood Grave - Horde Defender Quest0'),
    (30, 19, 5, 'Iceblood Grave - Horde Defender Quest1'),
    (30, 19, 6, 'Iceblood Grave - Horde Defender Quest2'),
    (30, 19, 7, 'Iceblood Grave - Horde Defender Quest3'),

    (30, 20, 0, 'Frostwolf Grave - Alliance Defender Quest0'),
    (30, 20, 1, 'Frostwolf Grave - Alliance Defender Quest1'),
    (30, 20, 2, 'Frostwolf Grave - Alliance Defender Quest2'),
    (30, 20, 3, 'Frostwolf Grave - Alliance Defender Quest3'),
    (30, 20, 4, 'Frostwolf Grave - Horde Defender Quest0'),
    (30, 20, 5, 'Frostwolf Grave - Horde Defender Quest1'),
    (30, 20, 6, 'Frostwolf Grave - Horde Defender Quest2'),
    (30, 20, 7, 'Frostwolf Grave - Horde Defender Quest3'),

    (30, 21, 0, 'Frostwolf Hut - Alliance Defender Quest0'),
    (30, 21, 1, 'Frostwolf Hut - Alliance Defender Quest1'),
    (30, 21, 2, 'Frostwolf Hut - Alliance Defender Quest2'),
    (30, 21, 3, 'Frostwolf Hut - Alliance Defender Quest3'),
    (30, 21, 4, 'Frostwolf Hut - Horde Defender Quest0'),
    (30, 21, 5, 'Frostwolf Hut - Horde Defender Quest1'),
    (30, 21, 6, 'Frostwolf Hut - Horde Defender Quest2'),
    (30, 21, 7, 'Frostwolf Hut - Horde Defender Quest3'),


    (30, 46, 0, 'North Mine - Alliance Boss'),
    (30, 46, 1, 'North Mine - Horde Boss'),
    (30, 46, 2, 'North Mine - Neutral Boss'),
    (30, 47, 0, 'South Mine - Alliance Boss'),
    (30, 47, 1, 'South Mine - Horde Boss'),
    (30, 47, 2, 'South Mine - Neutral Boss'),

    (30, 48, 0, 'Alliance Captain'),
    (30, 49, 0, 'Horde Captain'),

    (30, 50, 0, 'North Mine - Alliance Control'),
    (30, 50, 1, 'North Mine - Horde Control'),
    (30, 50, 2, 'North Mine - Neutral Control'),
    (30, 51, 0, 'South Mine - Alliance Control'),
    (30, 51, 1, 'South Mine - Horde Control'),
    (30, 51, 2, 'South Mine - Neutral Control'),

    (30, 52, 0, 'Alliance Marshal - Dunbaldar South'),
    (30, 53, 0, 'Alliance Marshal - Dunbaldar North'),
    (30, 54, 0, 'Alliance Marshal - Icewing Bunker'),
    (30, 55, 0, 'Alliance Marshal - Stoneheart Bunker'),

    (30, 56, 0, 'Horde Marshal - Iceblood Tower'),
    (30, 57, 0, 'Horde Marshal - Towerpoint'),
    (30, 58, 0, 'Horde Marshal - East Frostwolf Tower'),
    (30, 59, 0, 'Horde Marshal - West Frostwolf Tower'),

    (30, 60, 0, 'Herald - that guy who yells all the time ;)'),

    (30, 61, 0, 'Alliance - Boss'),
    (30, 62, 0, 'Horde - Boss'),

    (30, 63, 0, 'Alliance - Captain Dead'),
    (30, 64, 0, 'Horde - Captain Dead');
