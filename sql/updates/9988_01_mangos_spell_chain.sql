ALTER TABLE db_version CHANGE COLUMN required_9978_01_mangos_spell_bonus_data required_9988_01_mangos_spell_chain bit;

DELETE FROM  `spell_chain` WHERE `spell_id` IN (61024,61316);
INSERT INTO `spell_chain` (`spell_id`, `prev_spell`, `first_spell`, `rank`, `req_spell`) VALUES
/*Dalaran Intellect*/
(61024,61024,61024,1,27126),
/*Dalaran Brilliance*/
(61316,61316,61316,1,27127);
