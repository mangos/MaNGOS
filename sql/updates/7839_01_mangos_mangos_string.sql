ALTER TABLE db_version CHANGE COLUMN required_7830_01_mangos_spell_chain required_7839_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN(171,283);

INSERT INTO mangos_string VALUES
(171,'You can\'t teleport self to self!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
