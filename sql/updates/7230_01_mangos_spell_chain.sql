ALTER TABLE db_version CHANGE COLUMN required_7214_02_mangos_mangos_string required_7230_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE  spell_id in (18220,18937,18938,27265,59092);
/*Dark Pact*/
INSERT INTO spell_chain VALUES
(18220,0,    18220,1,0),
(18937,18220,18220,2,0),
(18938,18937,18220,3,0),
(27265,18938,18220,4,0),
(59092,27265,18220,5,0);
