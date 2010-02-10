ALTER TABLE db_version CHANGE COLUMN required_8726_01_mangos_spell_proc_event required_8731_01_mangos_creature_template bit;

ALTER TABLE creature_template
  CHANGE COLUMN heroic_entry difficulty_entry_1 mediumint(8) unsigned NOT NULL default '0';
