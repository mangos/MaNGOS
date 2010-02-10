ALTER TABLE db_version CHANGE COLUMN required_8498_01_mangos_spell_proc_event required_8499_01_mangos_spell_elixir bit;

/* Elexirs added in 3.x */
DELETE FROM `spell_elixir` WHERE `entry` IN
(53747,53748,53746,53749,53751,53763,53764,54452,54494,54497,60340,60341,60343,60344,60345,60346,60347);

INSERT INTO `spell_elixir` (`entry`, `mask`) VALUES
(53747,0x2),
(53748,0x1),
(53746,0x1),
(53749,0x1),
(53751,0x2),
(53763,0x2),
(53764,0x2),
(54452,0x1),
(54494,0x1),
(54497,0x2),
(60340,0x1),
(60341,0x1),
(60343,0x2),
(60344,0x1),
(60345,0x1),
(60346,0x1),
(60347,0x2);
