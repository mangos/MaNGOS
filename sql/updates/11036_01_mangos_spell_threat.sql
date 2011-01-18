ALTER TABLE db_version CHANGE COLUMN required_11023_01_mangos_spell_threat required_11036_01_mangos_spell_threat bit;

-- wrong pet spells, have EFFECT_THREAT
DELETE FROM spell_threat WHERE entry IN (11775,14921,24583);
-- Thunder Clap, x1.75 for all ranks
DELETE FROM spell_threat WHERE entry IN (6343,8198,8204,8205,11580,11581,25264,47501,47502);
-- Devastate, 5% AP for all ranks
DELETE FROM spell_threat WHERE entry IN (20243,30016,30022,47497,47498);
-- new: Swipe (Bear), Searing Pain, Death and Decay, Rune Strike, Heroic Throw
DELETE FROM spell_threat WHERE entry IN (779,5676,52212,56815,57755);

INSERT INTO spell_threat VALUES
(  779,   0, 1.50, 0.0), -- Swipe (Bear)
( 5676,   0, 2.00, 0.0), -- Searing Pain
( 6343,   0, 1.75, 0.0), -- Thunder Clap
(20243,   0, 1.00, 0.05), -- Devastate (Rank1)
(52212,   0, 1.90, 0.0), -- Death and Decay
(56815,   0, 1.75, 0.0), -- Rune Strike
(57755,   0, 1.50, 0.0); -- Heroic Throw

-- Lacerate
DELETE FROM spell_threat WHERE entry IN (33745,48567,48568);
INSERT INTO spell_threat VALUES
(33745, 182, 0.50, 0.0),
(48567, 409, 0.50, 0.0),
(48568, 515, 0.50, 0.0);

-- Maul
DELETE FROM spell_threat WHERE entry IN (6807,6808,6809,8972,9745,9880,9881,26996,48479,48480);
INSERT INTO spell_threat VALUES
( 6807,  13, 1.00, 0.0),
( 6808,  20, 1.00, 0.0),
( 6809,  27, 1.00, 0.0),
( 8972,  47, 1.00, 0.0),
( 9745,  75, 1.00, 0.0),
( 9880, 106, 1.00, 0.0),
( 9881, 140, 1.00, 0.0),
(26996, 212, 1.00, 0.0),
(48479, 345, 1.00, 0.0),
(48480, 422, 1.00, 0.0);

-- Demoralizing Roar
DELETE FROM spell_threat WHERE entry IN (99,1735,9490,9747,9898,26998,48559,48560);
INSERT INTO spell_threat VALUES
(   99, 15, 1.00, 0.0),
( 1735, 25, 1.00, 0.0),
( 9490, 29, 1.00, 0.0),
( 9747, 36, 1.00, 0.0),
( 9898, 42, 1.00, 0.0),
(26998, 49, 1.00, 0.0),
(48559, 54, 1.00, 0.0),
(48560, 62, 1.00, 0.0);
