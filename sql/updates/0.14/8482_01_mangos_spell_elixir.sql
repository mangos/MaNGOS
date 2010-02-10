ALTER TABLE db_version CHANGE COLUMN required_8462_01_mangos_creature_ai_texts required_8482_01_mangos_spell_elixir bit;

DELETE FROM `spell_elixir` WHERE `entry` IN
(53752,53755,53758,53760,54212,62380);

/* Flasks added in 3.x */
INSERT INTO `spell_elixir` (`entry`, `mask`) VALUES
(53752,0x3),
(53755,0x3),
(53758,0x3),
(53760,0x3),
(54212,0x3),
(62380,0x3);
