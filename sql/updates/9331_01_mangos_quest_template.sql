ALTER TABLE db_version CHANGE COLUMN required_9329_01_mangos_spell_chain required_9331_01_mangos_quest_template bit;

ALTER TABLE quest_template ADD COLUMN RewRepValueId1 tinyint(3) NOT NULL default '0' AFTER RewRepFaction5;
ALTER TABLE quest_template ADD COLUMN RewRepValueId2 tinyint(3) NOT NULL default '0' AFTER RewRepValueId1;
ALTER TABLE quest_template ADD COLUMN RewRepValueId3 tinyint(3) NOT NULL default '0' AFTER RewRepValueId2;
ALTER TABLE quest_template ADD COLUMN RewRepValueId4 tinyint(3) NOT NULL default '0' AFTER RewRepValueId3;
ALTER TABLE quest_template ADD COLUMN RewRepValueId5 tinyint(3) NOT NULL default '0' AFTER RewRepValueId4;
