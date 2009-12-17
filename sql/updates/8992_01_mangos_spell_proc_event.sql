ALTER TABLE db_version CHANGE COLUMN required_8988_01_mangos_spell_proc_event required_8992_01_mangos_spell_proc_event bit;

/*Ferocious Inspiration*/
DELETE FROM `spell_proc_event` WHERE `entry` IN (34457);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(34457, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000, 0);

/*Frenzy*/
DELETE FROM `spell_proc_event` WHERE `entry` IN (20784);
INSERT INTO `spell_proc_event` (`entry` ,`SchoolMask` ,`SpellFamilyName` ,`SpellFamilyMask0` ,`SpellFamilyMask1` ,`SpellFamilyMask2` ,`procFlags` ,`procEx` ,`ppmRate` ,`CustomChance` ,`Cooldown`)VALUES
(20784, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0.000000, 0.000000, 0);

/*Master Tactician*/
DELETE FROM `spell_proc_event` WHERE `entry` IN (34506, 34507, 34508, 34838, 34839);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(34506, 0x00000000,  9, 0x0007FA01, 0x00801081, 0x08000201, 0x00000000, 0x00000000, 0.000000, 0.000000, 0);

/*Hunting Party*/
DELETE FROM `spell_proc_event` WHERE entry IN (53290, 53291, 53292);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(53290, 0x00000000,  9, 0x00000800, 0x00000001, 0x00000200, 0x00000000, 0x00000002, 0.000000, 0.000000, 0);

/*Bloodsurge*/
DELETE FROM `spell_proc_event` WHERE `entry` IN (46913, 46914, 46915);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `cooldown`) VALUES
(46913, 0x00000000,  4, 0x00000040, 0x00000404, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 0);

/*Entrapment*/
DELETE FROM `spell_proc_event` WHERE `entry` IN (19184, 19387, 19388);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(19184, 0x00000000,  9, 0x00000010, 0x00002000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 0);

/*Concussive Barrage*/
DELETE FROM `spell_proc_event` WHERE `entry` IN (35100, 35102);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(35100, 0x00000000,  9, 0x00001000, 0x00000000, 0x00000001, 0x00000100, 0x00000000, 0.000000, 0.000000, 0);

/*Improved Stormstrike*/
DELETE FROM `spell_proc_event` WHERE `entry` IN (51521, 51522);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(51521, 0x00000000,  0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0.000000, 0.000000, 1);
