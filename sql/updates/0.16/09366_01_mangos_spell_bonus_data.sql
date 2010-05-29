ALTER TABLE db_version CHANGE COLUMN required_9331_01_mangos_quest_template required_9366_01_mangos_spell_bonus_data bit;

ALTER TABLE spell_bonus_data
  CHANGE COLUMN entry entry mediumint(8) unsigned NOT NULL;

DELETE FROM spell_bonus_data WHERE entry = 71824;
INSERT INTO spell_bonus_data VALUES
(71824,0,0,0,'Item - Shaman T9 Elemental 4P Bonus');
