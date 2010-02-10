ALTER TABLE db_version CHANGE COLUMN required_7988_09_mangos_spell_proc_event required_8016_01_mangos_npc_spellclick_spells bit;

ALTER TABLE npc_spellclick_spells
  CHANGE COLUMN quest_id quest_start mediumint(8) unsigned NOT NULL COMMENT 'reference to quest_template',
  ADD COLUMN    quest_start_active   tinyint(1) unsigned NOT NULL default '0' AFTER quest_start,
  ADD COLUMN    quest_end            mediumint(8) unsigned NOT NULL default '0' AFTER quest_start_active;

/* compatibility with old data */
UPDATE npc_spellclick_spells
  SET quest_end = quest_start, quest_start_active = 1
  WHERE quest_start <> 0;
