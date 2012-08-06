ALTER TABLE db_version CHANGE COLUMN required_10629_01_mangos_mangos_string required_10654_01_mangos_game_event_creature_quest bit;

ALTER TABLE game_event_creature_quest DROP PRIMARY KEY;
ALTER TABLE game_event_creature_quest ADD PRIMARY KEY (id,quest,event);
