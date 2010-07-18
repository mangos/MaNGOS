ALTER TABLE db_version CHANGE COLUMN required_10208_01_mangos_playercreateinfo required_10217_01_mangos_playercreateinfo_spell bit;

DELETE FROM playercreateinfo_spell WHERE spell=1843 ;
DELETE FROM playercreateinfo_spell WHERE spell=21084;
-- humans
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(1, 1, 68398, 'Opening'),
(1, 2, 20154, 'Seal of Righteousness'),
(1, 2, 68398, 'Opening'),
(1, 4, 68398, 'Opening'),
(1, 5, 68398, 'Opening'),
(1, 8, 68398, 'Opening'),
(1, 8, 71761, 'Deep Freeze Immunity State'),
(1, 9, 18822, 'Improved Enslave Demon'),
(1, 9, 68398, 'Opening'),
(1, 9, 75445, 'Demonic Immolate');

-- dwarf
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(3, 1, 68398, 'Opening'),
(3, 2, 20154, 'Seal of Righteousness'),
(3, 2, 68398, 'Opening'),
(3, 3, 68398, 'Opening'),
(3, 4, 68398, 'Opening'),
(3, 5, 68398, 'Opening');

-- night elfs
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(4, 1, 21009, 'Elusiveness'),
(4, 1, 68398, 'Opening'),
(4, 3, 21009, 'Elusiveness'),
(4, 3, 68398, 'Opening'),
(4, 4, 21009, 'Elusiveness'),
(4, 4, 68398, 'Opening'),
(4, 5, 21009, 'Elusiveness'),
(4, 5, 68398, 'Opening'),
(4, 11, 66530, 'Improved Barkskin (Passive)'),
(4, 11, 68398, 'Opening');

-- gnome
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(7, 1, 68398, 'Opening'),
(7, 4, 68398, 'Opening'),
(7, 8, 68398, 'Opening'),
(7, 8, 71761, 'Deep Freeze Immunity State'),
(7, 9, 18822, 'Improved Enslave Demon'),
(7, 9, 68398, 'Opening'),
(7, 9, 75445, 'Demonic Immolate');

-- orc
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(2, 1, 68398, 'Opening'),
(2, 3, 68398, 'Opening'),
(2, 4, 68398, 'Opening'),
(2, 7, 65222, 'Command'),
(2, 7, 68398, 'Opening'),
(2, 7, 75461, 'Flame Shock Passive'),
(2, 9, 18822, 'Improved Enslave Demon'),
(2, 9, 68398, 'Opening'),
(2, 9, 75445, 'Demonic Immolate');

-- undead
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(5, 1, 68398, 'Opening'),
(5, 4, 68398, 'Opening'),
(5, 5, 68398, 'Opening'),
(5, 8, 68398, 'Opening'),
(5, 8, 71761, 'Deep Freeze Immunity State'),
(5, 9, 18822, 'Improved Enslave Demon'),
(5, 9, 68398, 'Opening'),
(5, 9, 75445, 'Demonic Immolate');

-- tauren 
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(6, 1, 68398, 'Opening'),
(6, 3, 68398, 'Opening'),
(6, 7, 68398, 'Opening'),
(6, 7, 75461, 'Flame Shock Passive'),
(6, 11, 66530, 'Improved Barkskin (Passive)'),
(6, 11, 68398, 'Opening');

-- troll
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(8, 1, 68398, 'Opening'),
(8, 3, 68398, 'Opening'),
(8, 4, 68398, 'Opening'),
(8, 5, 68398, 'Opening'),
(8, 7, 68398, 'Opening'),
(8, 7, 75461, 'Flame Shock Passive'),
(8, 8, 68398, 'Opening'),
(8, 8, 71761, 'Deep Freeze Immunity State');

-- bloodelf
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(10, 2, 20154, 'Seal of Righteousness'),
(10, 2, 68398, 'Opening'),
(10, 3, 68398, 'Opening'),
(10, 4, 68398, 'Opening'),
(10, 5, 68398, 'Opening'),
(10, 8, 68398, 'Opening'),
(10, 9, 18822, 'Improved Enslave Demon'),
(10, 9, 68398, 'Opening'),
(10, 9, 75445, 'Demonic Immolate');

-- drarenei
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(11, 1, 68398, 'Opening'),
(11, 2, 20154, 'Seal of Righteousness'),
(11, 2, 68398, 'Opening'),
(11, 3, 68398, 'Opening'),
(11, 5, 68398, 'Opening'),
(11, 7, 68398, 'Opening'),
(11, 7, 75461, 'Flame Shock Passive'),
(11, 8, 68398, 'Opening'),
(11, 8, 71761, 'Deep Freeze Immunity State');

-- dk 
INSERT IGNORE INTO playercreateinfo_spell (race, class, Spell, Note) VALUES 
(1, 6, 68398, 'Opening'),
(2, 6, 68398, 'Opening'),
(3, 6, 68398, 'Opening'),
(4, 6, 68398, 'Opening'),
(5, 6, 68398, 'Opening'),
(6, 6, 68398, 'Opening'),
(7, 6, 68398, 'Opening'),
(8, 6, 68398, 'Opening'),
(10, 6, 68398, 'Opening'),
(11, 6, 68398, 'Opening');
