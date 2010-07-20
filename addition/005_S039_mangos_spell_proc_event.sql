DELETE FROM `spell_proc_event` WHERE `entry` IN (48492,48494,48495);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskA1`, `SpellFamilyMaskA2`, `SpellFamilyMaskB0`, `SpellFamilyMaskB1`, `SpellFamilyMaskB2`, `SpellFamilyMaskC0`, `SpellFamilyMaskC1`, `SpellFamilyMaskC2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(48492, 0, 7, 524288, 0, 2048, 0, 0, 0, 0, 0, 0, 16384, 0, 0, 0, 0),
(48494, 0, 7, 524288, 0, 2048, 0, 0, 0, 0, 0, 0, 16384, 0, 0, 0, 0),
(48495, 0, 7, 524288, 0, 2048, 0, 0, 0, 0, 0, 0, 16384, 0, 0, 0, 0);
