ALTER TABLE db_version CHANGE COLUMN required_8361_01_mangos_spell_bonus_data required_8364_01_mangos_db_version bit;

ALTER TABLE db_version
  ADD COLUMN cache_id int(10) default '0' AFTER creature_ai_version;
