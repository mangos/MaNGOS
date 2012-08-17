ALTER TABLE db_version CHANGE COLUMN required_12091_01_mangos_spell_template required_12093_01_mangos_spell_template bit;

DELETE FROM spell_template WHERE id IN (44920, 44924, 44928, 44932, 45158, 45162, 45166, 45170);
INSERT INTO spell_template VALUES
(44920, 0x00000000, 101,  21,   6,   1,   0,  56, 24941,  0,     'Model - Shattered Sun Marksman - BE Male Tier 4'),
(44924, 0x00000000, 101,  21,   6,   1,   0,  56, 24945,  0,     'Model - Shattered Sun Marksman - BE Female Tier 4'),
(44928, 0x00000000, 101,  21,   6,   1,   0,  56, 24949,  0,     'Model - Shattered Sun Marksman - Draenei Male Tier 4'),
(44932, 0x00000000, 101,  21,   6,   1,   0,  56, 24953,  0,     'Model - Shattered Sun Marksman - Draenei Female Tier 4'),
(45158, 0x00000000, 101,  21,   6,   1,   0,  56, 25119,  0,     'Model - Shattered Sun Warrior - BE Female Tier 4'),
(45162, 0x00000000, 101,  21,   6,   1,   0,  56, 25123,  0,     'Model - Shattered Sun Warrior - BE Male Tier 4'),
(45166, 0x00000000, 101,  21,   6,   1,   0,  56, 25127,  0,     'Model - Shattered Sun Warrior - Draenei Female Tier 4'),
(45170, 0x00000000, 101,  21,   6,   1,   0,  56, 25131,  0,     'Model - Shattered Sun Warrior - Draenei Male Tier 4');
