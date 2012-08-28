ALTER TABLE character_db_version CHANGE COLUMN required_11391_01_characters_auction required_11436_01_characters_character_queststatus bit;

ALTER TABLE character_queststatus
  ADD COLUMN itemcount5 int(11) unsigned NOT NULL default '0' AFTER itemcount4,
  ADD COLUMN itemcount6 int(11) unsigned NOT NULL default '0' AFTER itemcount5;

