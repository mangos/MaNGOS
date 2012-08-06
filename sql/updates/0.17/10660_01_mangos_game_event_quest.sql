ALTER TABLE db_version CHANGE COLUMN required_10654_01_mangos_game_event_creature_quest required_10660_01_mangos_game_event_quest bit;

DROP TABLE IF EXISTS game_event_quest;
CREATE TABLE game_event_quest (
  quest mediumint(8) unsigned NOT NULL default '0' COMMENT 'entry from quest_template',
  event smallint(5) unsigned NOT NULL default '0' COMMENT 'entry from game_event',
  PRIMARY KEY  (quest,event)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Game event system';

INSERT INTO game_event_quest SELECT DISTINCT quest, event FROM game_event_creature_quest;

DROP TABLE game_event_creature_quest;
