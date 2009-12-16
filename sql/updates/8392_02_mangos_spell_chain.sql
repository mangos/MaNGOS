ALTER TABLE db_version CHANGE COLUMN required_8392_01_mangos_spell_proc_event required_8392_02_mangos_spell_chain bit;

DELETE FROM `spell_chain` WHERE `spell_id` IN (47569,47570);
INSERT INTO `spell_chain` (`spell_id`, `prev_spell`, `first_spell`, `rank`, `req_spell`) VALUES
/*Improved Shadowform*/
(47569,0,47569,1,0),
(47570,47569,47569,2,0);
