ALTER TABLE db_version CHANGE COLUMN required_12120_01_mangos_spell_template required_12121_01_mangos_spell_template bit;

DELETE FROM spell_template WHERE id IN (34810, 34817, 34818, 34819, 35153, 35904, 35905, 35906);
INSERT INTO spell_template VALUES
(34810, 0x00000000, 101,  21,  28,  42,   8,   0, 20083,  64,    0,     'Summon Summoned Bloodwarder Mender behind of the caster'),
(34817, 0x00000000, 101,  21,  28,  44,   8,   0, 20078,  64,    0,     'Summon Summoned Bloodwarder Reservist right of the caster'),
(34818, 0x00000000, 101,  21,  28,  43,   8,   0, 20078,  64,    0,     'Summon Summoned Bloodwarder Reservist left of the caster'),
(34819, 0x00000000, 101,  21,  28,  41,   8,   0, 20078,  64,    0,     'Summon Summoned Bloodwarder Reservist front of the caster'),
(35153, 0x00000000, 101,  21,  28,  42,   8,   0, 20405,  64,    0,     'Summon Nether Charge behind of the caster'),
(35904, 0x00000000, 101,  21,  28,  44,   8,   0, 20405,  64,    0,     'Summon Nether Charge right of the caster'),
(35905, 0x00000000, 101,  21,  28,  43,   8,   0, 20405,  64,    0,     'Summon Nether Charge left of the caster'),
(35906, 0x00000000, 101,  21,  28,  41,   8,   0, 20405,  64,    0,     'Summon Nether Charge front of the caster');
