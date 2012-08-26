ALTER TABLE db_version CHANGE COLUMN required_11385_01_mangos_creature_template required_11433_01_mangos_item_template bit;

UPDATE item_template
  SET ScriptName = '' WHERE ScriptName = 'internalItemHandler';

