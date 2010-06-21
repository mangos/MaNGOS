ALTER TABLE db_version CHANGE COLUMN required_10086_01_mangos_command required_10089_01_mangos_game_event_pool bit;

DROP TABLE IF EXISTS `game_event_pool`;
