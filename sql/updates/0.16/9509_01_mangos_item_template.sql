ALTER TABLE db_version CHANGE COLUMN required_9482_01_mangos_spell_proc_event required_9509_01_mangos_item_template bit;

alter table item_template
  add column NonConsumable tinyint(1) UNSIGNED DEFAULT '0' NOT NULL after maxMoneyLoot;

update item_template
  set NonConsumable = 1 WHERE ItemLimitCategory = 4;
