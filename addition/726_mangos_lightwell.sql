DELETE FROM `creature_template_addon` WHERE `entry` IN (31897, 31896, 31895, 31894, 31893, 31883);
INSERT INTO `creature_template_addon` (`entry`,`auras`) VALUES 
('31897','59907 0'),
('31896','59907 0'),
('31895','59907 0'),
('31894','59907 0'),
('31893','59907 0'),
('31883','59907 0');

UPDATE `creature_template` SET `unit_flags` = 4, `flags_extra` = 2 WHERE `entry` IN (31897, 31896, 31895, 31894, 31893, 31883);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (31897, 31896, 31895, 31894, 31893, 31883);
INSERT INTO `npc_spellclick_spells` (`npc_entry`,`spell_id`,`quest_start`,`cast_flags`) VALUES 
('31897','60123','0','2'),
('31896','60123','0','2'),
('31895','60123','0','2'),
('31894','60123','0','2'),
('31893','60123','0','2'),
('31883','60123','0','2');

DELETE FROM `spell_proc_event` WHERE entry in (59907);
INSERT INTO `spell_proc_event` VALUES
(59907, 0x00,  0x06, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00004000, 0x00004000, 0x00004000, 0x00000000, 0x00000000, 0.000000, 0.000000,  0);