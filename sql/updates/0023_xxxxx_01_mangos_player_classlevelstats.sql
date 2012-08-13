ALTER TABLE db_version CHANGE COLUMN required_0001_xxxxx_01_mangos required_0023_xxxxx_01_mangos_player_classlevelstats bit;

DROP TABLE IF EXISTS `player_classlevelstats`;
