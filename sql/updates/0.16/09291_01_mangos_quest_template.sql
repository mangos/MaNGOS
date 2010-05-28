ALTER TABLE db_version CHANGE COLUMN required_9289_01_mangos_spell_proc_event required_9291_01_mangos_quest_template bit;

ALTER TABLE quest_template ADD COLUMN CompletedText text AFTER EndText;
