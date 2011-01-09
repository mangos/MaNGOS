ALTER TABLE db_version CHANGE COLUMN required_10973_01_mangos_game_event_mail required_10988_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (65,66);

INSERT INTO mangos_string VALUES
(65,'Using script library: <Unknown Script Library>',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(66,'Using script library: <No Script Library Loaded>',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
