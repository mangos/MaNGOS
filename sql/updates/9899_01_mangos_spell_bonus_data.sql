ALTER TABLE db_version CHANGE COLUMN required_9891_02_mangos_creature_movement_scripts required_9899_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` IN (
  17,122,139,421,589,774,1064,1449,2060,2061,2136,2912,2948,8004,
  11426,19236,25912,30451,32379,32546,33110,34861,42463,44457,
  49821,51505,53739,61391);

INSERT INTO `spell_bonus_data` VALUES
(53739, 0,     0, 0.003, 'Paladin - Seal of Corruption (full stack proc)'),
(42463, 0,     0, 0.003, 'Paladin - Seal of Vengeance (full stack proc)'),
(49821, 0.2857,0, 0,     'Priest - Mind Sear Trigger');
