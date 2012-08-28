ALTER TABLE db_version CHANGE COLUMN required_12087_02_mangos_command required_12091_01_mangos_spell_template bit;

DELETE FROM spell_template WHERE id IN (26133);
INSERT INTO spell_template VALUES
(26133, 0x00000000, 101,  21,  76,  18,   0,   0, 180795, 0,     'Summon Sandworm Base');
