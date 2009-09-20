ALTER TABLE db_version CHANGE COLUMN required_8511_01_mangos_spell_proc_event required_8514_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` IN (54158);

INSERT INTO `spell_bonus_data` VALUES
(54158, 0.25, 0, 0, 'Paladin - Judgement');
