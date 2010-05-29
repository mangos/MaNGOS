ALTER TABLE db_version CHANGE COLUMN required_9297_01_mangos_item_template required_9309_01_mangos_quest_template bit;

ALTER TABLE quest_template ADD COLUMN RewXPId tinyint(3) unsigned NOT NULL default '0' AFTER NextQuestInChain;
