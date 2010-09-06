DELETE FROM `creature_template_addon` WHERE `entry` IN (89);
INSERT INTO `creature_template_addon` (`entry`,`auras`) VALUES
('89','39007 0');
-- Warlock pets
DELETE FROM `creature_template_addon` WHERE `entry` IN (416,417);
INSERT INTO `creature_template_addon` (`entry`,`auras`) VALUES
('416', '34947 0 34956 0 34957 0 34958 0'),
('417', '34947 0 34956 0 34957 0 34958 0');
