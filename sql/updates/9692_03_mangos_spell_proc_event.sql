ALTER TABLE db_version CHANGE COLUMN required_9690_01_mangos_spell_proc_event required_9692_03_mangos_spell_proc_event bit;

-- Rampage: now passive instead of being a proc
DELETE FROM `spell_proc_event` WHERE `entry` = 29801;
-- Unleashed Rage: now passive instead of being a proc
DELETE FROM `spell_proc_event` WHERE `entry` IN (30802, 30803, 30804, 30805, 30808, 30809);
-- Endless Winter: now passive instead of being a proc
DELETE FROM `spell_proc_event` WHERE `entry` IN (49137, 49657);
-- Elemental Oath: now passive instead of being a proc
DELETE FROM `spell_proc_event` WHERE `entry` IN (51466);
-- Abomination's Might: now passive instead of being a proc
DELETE FROM `spell_proc_event` WHERE `entry` IN (53137, 53138);
-- Incanter's Absorption: now triggering at specific spells absorbing.
DELETE FROM `spell_proc_event` WHERE `entry` IN (44394, 44395, 44396);
-- Ferocious Inspiration: now passive instead of being a proc
DELETE FROM `spell_proc_event` WHERE `entry` = 34457;
