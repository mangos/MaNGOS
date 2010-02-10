ALTER TABLE db_version CHANGE COLUMN required_8589_07_mangos_spell_elixir required_8589_08_mangos_item_template bit;

ALTER TABLE item_template
  CHANGE COLUMN ItemLevel ItemLevel smallint(5) unsigned NOT NULL DEFAULT 0;
