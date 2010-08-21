-- Water totems:               draenei          orc             tauren          troll

UPDATE creature_template SET modelid_1=19075, modelid_2=30759, modelid_3=4587, modelid_4=30763 WHERE entry IN (
3527, -- Healing Stream Totem
3573, -- Mana Spring Totem
3907, -- Healing Stream Totem III
3909, -- Healing Stream Totem V
5927, -- Fire Resistance Totem
7414, -- Mana Spring Totem II
7415, -- Mana Spring Totem III
7416, -- Mana Spring Totem IV
7424, -- Fire Resistance Totem II
7425, -- Fire Resistance Totem III
10467, -- Mana Tide Totem
15489, -- Mana Spring Totem V
15488, -- Healing Stream Totem VI
31186, -- Mana Spring Totem VI
31169, -- Fire Resistance Totem V
31170, -- Fire Resistance Totem VI
31181, -- Healing Stream Totem VII
31182, -- Healing Stream Totem VIII
31185, -- Healing Stream Totem IX
31189, -- Mana Spring Totem VII
31190, -- Mana Spring Totem VIII
3906, -- Healing Stream Totem II
3908, -- Healing Stream Totem IV
5923, -- Poison Cleansing Totem
5924, -- Cleansing Totem
11100, -- Mana Tide Totem II
11101, -- Mana Tide Totem III
15487, -- Fire Resistance Totem IV
17061 -- Mana Tide Totem IV
);
-- Earth totems:               draenei          orc             tauren          troll
UPDATE creature_template SET modelid_1=19073, modelid_2=30757, modelid_3=4588, modelid_4=30761 WHERE entry IN (
2630, -- Earthbind Totem
3579, -- Stoneclaw Totem
3911, -- Stoneclaw Totem II
3912, -- Stoneclaw Totem III
3913, -- Stoneclaw Totem IV
7398, -- Stoneclaw Totem V
7399, -- Stoneclaw Totem VI
15478, -- Stoneclaw Totem VII
31120, -- Stoneclaw Totem VIII
31121, -- Stoneclaw Totem IX
31122, -- Stoneclaw Totem X
5873, -- Stoneskin Totem
5919, -- Stoneskin Totem II
5920, -- Stoneskin Totem III
7366, -- Stoneskin Totem IV
7367, -- Stoneskin Totem V
7368, -- Stoneskin Totem VI
15470, -- Stoneskin Totem VII
15474, -- Stoneskin Totem VIII
31175, -- Stoneskin Totem IX
31176, -- Stoneskin Totem X
15430, -- Earth Elemental Totem
5874, -- Strength of Earth Totem
5921, -- Strength of Earth Totem II
5922, -- Strength of Earth Totem III
7403, -- Strength of Earth Totem IV
15464, -- Strength of Earth Totem V
15479, -- Strength of Earth Totem VI
30647, -- Strength of Earth Totem VII
31129, -- Strength of Earth Totem VIII
5913 -- Tremor Totem
);
-- Fire totems:               draenei          orc             tauren          troll
UPDATE creature_template SET modelid_1=19074, modelid_2=30758, modelid_3=4589, modelid_4=30762 WHERE entry IN (
5929, -- Magma Totem
7464, -- Magma Totem II
7465, -- Magma Totem III
7466, -- Magma Totem IV
15484, -- Magma Totem V
31166, -- Magma Totem VI
31167, -- Magma Totem VII
2523, -- Searing Totem
3902, -- Searing Totem II
3903, -- Searing Totem III
3904, -- Searing Totem IV
7400, -- Searing Totem V
7402, -- Searing Totem VI
15480, -- Searing Totem VII
31162, -- Searing Totem VIII
31164, -- Searing Totem IX
31165, -- Searing Totem X
5926, -- Frost Resistance Totem
7412, -- Frost Resistance Totem II
7413, -- Frost Resistance Totem III
15486, -- Frost Resistance Totem IV
31171, -- Frost Resistance Totem V
31172, -- Frost Resistance Totem VI
5950, -- Flametongue Totem
6012, -- Flametongue Totem II
7423, -- Flametongue Totem III
10557, -- Flametongue Totem IV
15485, -- Flametongue Totem V
31132, -- Flametongue Totem VI
31133, -- Flametongue Totem VIII
31158, -- Flametongue Totem VII
15439, -- Fire Elemental Totem
17539, -- Totem of Wrath I
30652, -- Totem of Wrath II
30653, -- Totem of Wrath III
30654, -- Totem of Wrath IV
5879, -- Fire Nova Totem
6110, -- Fire Nova Totem II
6111, -- Fire Nova Totem III
7844, -- Fire Nova Totem IV
7845, -- Fire Nova Totem V
15482, -- Fire Nova Totem VI
32775, -- Fire Nova Totem IX
32776 -- Fire Nova Totem VIII
);
-- Air totems:               draenei            orc             tauren         troll
UPDATE creature_template SET modelid_1=19071, modelid_2=30756, modelid_3=4590, modelid_4=30760 WHERE entry IN (
3968, -- Sentry Totem
6112, -- Windfury Totem
7467, -- Nature Resistance Totem
7468, -- Nature Resistance Totem II
15447, -- Wrath of Air Totem
15490, -- Nature Resistance Totem IV
31173, -- Nature Resistance Totem V
31174, -- Nature Resistance Totem VI
5925, -- Grounding Totem
7469, -- Nature Resistance Totem III
7483, -- Windfury Totem II
7484, -- Windfury Totem III
15496, -- Windfury Totem IV
15497 -- Windfury Totem V
);
DELETE FROM creature_model_info WHERE modelid IN (
-- orco
30756, 30757, 30758, 30759,
-- troll
30760, 30761, 30762, 30763);
INSERT INTO `creature_model_info` (`modelid`, `bounding_radius`, `combat_reach`, `gender`, `modelid_other_gender`) VALUES
-- orco
(30756, 1, 1, 2, 0),
(30757, 1, 1, 2, 0),
(30758, 1, 1, 2, 0),
(30759, 1, 1, 2, 0),
-- troll
(30760, 1, 1, 2, 0),
(30761, 1, 1, 2, 0),
(30762, 1, 1, 2, 0),
(30763, 1, 1, 2, 0);
