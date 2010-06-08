ALTER TABLE db_version CHANGE COLUMN required_9716_01_mangos_npc_vendor required_9716_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry in (210);

INSERT INTO mangos_string VALUES
(210,'Item \'%i\' (with extended cost %u) already in vendor list.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
