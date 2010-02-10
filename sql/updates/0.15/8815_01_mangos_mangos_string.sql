ALTER TABLE db_version CHANGE COLUMN required_8803_02_mangos_playercreateinfo_action required_8815_01_mangos_mangos_string bit;

-- this sql might delete some of your translated strings, if you translate them

DELETE FROM mangos_string WHERE entry in (718, 719);

INSERT INTO mangos_string VALUES (718,'|cffff0000[Arena Queue Announcer]:|r All Arenas -- Joined : %ux%u : %u|r',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
INSERT INTO mangos_string VALUES (719,'|cffff0000[Arena Queue Announcer]:|r All Arenas -- Left : %ux%u : %u|r',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
