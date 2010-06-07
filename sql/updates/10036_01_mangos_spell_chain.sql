ALTER TABLE db_version CHANGE COLUMN required_10017_01_mangos_spell_proc_event required_10036_01_mangos_spell_chain bit;

DELETE FROM  `spell_chain` WHERE `spell_id` IN (30881,30883,30884,30885,30886);

INSERT INTO `spell_chain` (`spell_id`, `prev_spell`, `first_spell`, `rank`, `req_spell`) VALUES
/*Nature's Guardian*/
(30881,0,30881,1,0),
(30883,30881,30881,2,0),
(30884,30883,30881,3,0),
(30885,30884,30881,4,0),
(30886,30885,30881,5,0);
