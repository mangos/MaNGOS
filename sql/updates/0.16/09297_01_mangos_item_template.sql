ALTER TABLE db_version CHANGE COLUMN required_9296_01_mangos_spell_chain required_9297_01_mangos_item_template bit;

ALTER TABLE item_template CHANGE COLUMN spellcharges_1 spellcharges_1 smallint(5) NOT NULL default '0';
ALTER TABLE item_template CHANGE COLUMN spellcharges_2 spellcharges_2 smallint(5) NOT NULL default '0';
ALTER TABLE item_template CHANGE COLUMN spellcharges_3 spellcharges_3 smallint(5) NOT NULL default '0';
ALTER TABLE item_template CHANGE COLUMN spellcharges_4 spellcharges_4 smallint(5) NOT NULL default '0';
ALTER TABLE item_template CHANGE COLUMN spellcharges_5 spellcharges_5 smallint(5) NOT NULL default '0';
