ALTER TABLE db_version CHANGE COLUMN required_7249_01_mangos_spell_proc_event required_7251_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE  spell_id in (52375,47541,49892,49893,49894,49895);
/*Dark Pact*/
INSERT INTO spell_chain VALUES
/*DeathCoil*/
(47541,0,47541,1,0),
(49892,47541,47541,2,0),
(49893,49892,47541,3,0),
(49894,49893,47541,4,0),
(49895,49894,47541,5,0);
