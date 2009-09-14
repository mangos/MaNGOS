ALTER TABLE db_version CHANGE COLUMN required_8488_01_mangos_spell_chain required_8488_02_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` IN
-- Spells that would be better off using default calculations (and should be removed from base MaNGOS tables)
(689, 30108, 6789, 29722, 5676, 686, 17877, 30283, 11113, 31661, 120, 25914, 596, 8092, 15407,
-- Spells that are getting entries below
18790, 42223, 27243, 30294, 47960, 47897, 44425, 42208, 19750, 635, 20167, 20267, 20187, 53600, 25997, 2944, 58381, 27813, 33619, 5570, 61391);

INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `comments`) VALUES
('18790', '0', '0', '0','Warlock - Fel Stamina'),
('42223', '0.285714', '0', '0', 'Warlock - Rain of Fire Triggered'), -- should be same as default calc (2/7), but this is a triggered spell
('27243', '0.2129', '0.25', '0', 'Warlock - Seed of Corruption'),
('30294', '0', '0', '0', 'Warlock - Soul Leech'),
('47960', '0', '0.06666', '0', 'Warlock - Shadowflame DoT'),
('47897', '0.1064', '0', '0', 'Warlock - Shadowflame Direct'),
('44425', '0.714286', '0', '0', 'Mage - Arcane Barrage'), -- treat as 2.5 second cast time (as of 3.0.9)
('42208', '0.1437', '0', '0', 'Mage - Blizzard Triggered'),
('19750', '1', '0', '0', 'Paladin - Flash of Light'),
('635', '1.66', '0', '0', 'Paladin - Holy Light'), -- These two Paladin heals are their default calculations multiplied by 7/3, not sure why that is
('20167', '0.15', '0', '0.15', 'Paladin - Seal of Light Proc'),
('20267', '0.1', '0', '0.1', 'Paladin - Judgement of Light Proc'),
('20187', '0.32', '0', '0', 'Paladin - Judgement of Righteousness'),
('53600', '0', '0', '0', 'Paladin - Shield of Righteousness'),
('25997', '0', '0', '0', 'Paladin - Eye for an Eye'),
('2944', '0', '0.1849', '0', 'Priest - Devouring Plague'),
('58381', '0.257143', '0', '0', 'Priest - Mind Flay Triggered'), -- Treated as 2.7 sec channel instead of 3?
('27813', '0', '0', '0', 'Priest - Blessed Recovery'),
('33619', '0', '0', '0', 'Priest - Reflective Shield'),
('5570', '0', '0.2', '0', 'Druid - Insect Swarm'),
('61391', '0.193', '0', '0', 'Druid - Typhoon');
