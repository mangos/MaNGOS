ALTER TABLE db_version CHANGE COLUMN required_9310_01_mangos_spell_elixir required_9312_01_mangos_quest_template bit;

ALTER TABLE quest_template ADD COLUMN RewHonorMultiplier float NOT NULL default '0' AFTER RewHonorableKills;
ALTER TABLE quest_template CHANGE COLUMN RewHonorableKills RewHonorAddition int unsigned NOT NULL default '0';
