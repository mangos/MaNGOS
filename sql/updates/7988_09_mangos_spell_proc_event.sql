ALTER TABLE db_version CHANGE COLUMN required_7988_08_mangos_spell_bonus_data required_7988_09_mangos_spell_proc_event bit;

/* Blackout removed */
DELETE FROM spell_proc_event WHERE entry IN (15268, 15269, 15323, 15324, 15325, 15326);
/* Improved Wing Clip removed */
DELETE FROM spell_proc_event WHERE entry IN (19228, 19232, 19233);
/* Shadow Mastery, not have charges now */
DELETE FROM spell_proc_event WHERE entry IN (17794,17797,17798,17799,17800);
/* Lightning Overload have 3 ranks now, 2 dropped */
DELETE FROM spell_proc_event WHERE entry IN (30680,30681);
/* Unleashed Rage have 3 ranks now, 2 dropped */
DELETE FROM spell_proc_event WHERE entry IN (30806,30807,30810,30811);
/* Concussive Barrage have 2 ranks now, 1 dropped */
DELETE FROM spell_proc_event WHERE entry IN (35103);
/* Demonic Empathy, removed */
DELETE FROM spell_proc_event WHERE entry IN (47232,47234,47235);
/* Rapture have 3 ranks now, 2 dropped */
DELETE FROM spell_proc_event WHERE entry IN (47538,47539);
/* Psychic Horror have 1 rank now, 1 dropped */
DELETE FROM spell_proc_event WHERE entry IN (47572);
/* Sudden Doom have 3 ranks now, 2 dropped */
DELETE FROM spell_proc_event WHERE entry IN (49531,49532);
/* Hunting Party have 3 ranks now, 2 dropped */
DELETE FROM spell_proc_event WHERE entry IN (53293,53294);
/* Righteous Vengeance have 3 ranks now, 2 dropped */
DELETE FROM spell_proc_event WHERE entry IN (53383,53384);
/* Night of the Dead not have charges now */
DELETE FROM spell_proc_event WHERE entry IN (55620,55623);
/* Pandemic have 1 rank, 2 dropped */
DELETE FROM spell_proc_event WHERE entry IN (58436,58437);
/* Improved Holy Concentration, removed */
DELETE FROM spell_proc_event WHERE entry IN (47549,47551,47552);
/* Serendipity, replace by aanother spell ids */
DELETE FROM spell_proc_event WHERE entry IN (47555,47556,47557);
/* T.N.T. non triggring now */
DELETE FROM spell_proc_event WHERE entry IN (56333,56336,56337);
