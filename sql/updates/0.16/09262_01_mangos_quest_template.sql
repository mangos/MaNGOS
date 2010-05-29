ALTER TABLE db_version CHANGE COLUMN required_9244_02_mangos_spell_chain required_9262_01_mangos_quest_template bit;

ALTER TABLE quest_template CHANGE COLUMN QuestFlags QuestFlags mediumint(8) UNSIGNED NOT NULL default '0';
