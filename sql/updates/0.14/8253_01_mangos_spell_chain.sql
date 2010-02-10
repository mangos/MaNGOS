ALTER TABLE db_version CHANGE COLUMN required_8251_03_mangos_spell_proc_event required_8253_01_mangos_spell_chain bit;

DELETE FROM `spell_chain` WHERE `spell_id` IN (32385,32387,32392,32393,32394,51528,51529,51530,51531,51532);
INSERT INTO `spell_chain` (`spell_id`, `prev_spell`, `first_spell`, `rank`, `req_spell`) VALUES
/*Shadow embrace*/
(32385,0,32385,1,0),
(32387,32385,32385,2,0),
(32392,32387,32385,3,0),
(32393,32392,32385,4,0),
(32394,32393,32385,5,0),
/*Maelstrom Weapon*/
(51528,0,51528,1,0),
(51529,51528,51528,2,0),
(51530,51529,51528,3,0),
(51531,51530,51528,4,0),
(51532,51531,51528,5,0);
