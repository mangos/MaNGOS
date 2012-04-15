ALTER TABLE db_version CHANGE COLUMN required_11958_01_mangos_mangos_string required_11964_01_mangos_conditions bit;

DROP TABLE IF EXISTS `conditions`;
CREATE TABLE `conditions` (
  `condition_entry` mediumint(8) unsigned NOT NULL auto_increment COMMENT 'Identifier',
  `type` tinyint(3) signed NOT NULL DEFAULT '0' COMMENT 'Type of the condition',
  `value1` mediumint(8) unsigned NOT NULL DEFAULT '0' COMMENT 'data field one for the condition',
  `value2` mediumint(8) unsigned NOT NULL DEFAULT '0' COMMENT 'data field two for the condition',
  PRIMARY KEY  (`condition_entry`),
  CONSTRAINT unique_conditions UNIQUE (type,value1,value2)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Condition System';

ALTER TABLE gossip_menu_option ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER cond_3_val_2;
ALTER TABLE gossip_menu ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER cond_2_val_2;

ALTER TABLE reference_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE creature_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE gameobject_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE pickpocketing_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE item_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE fishing_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE skinning_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE disenchant_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE mail_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE prospecting_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE spell_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;
ALTER TABLE milling_loot_template ADD COLUMN `condition_id` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' AFTER condition_value2;

-- convert *_loot_template conditions -- Note to DB-devs If you create a dump for the new system in an updatepack, these queries are not required
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM reference_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM creature_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM gameobject_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM pickpocketing_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM item_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM fishing_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM skinning_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM disenchant_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM mail_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM prospecting_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM spell_loot_template WHERE lootcondition>0;
INSERT IGNORE INTO conditions (type, value1, value2) SELECT lootcondition, condition_value1, condition_value2 FROM milling_loot_template WHERE lootcondition>0;

UPDATE reference_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE creature_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE gameobject_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE pickpocketing_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE item_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE fishing_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE skinning_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE disenchant_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE mail_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE prospecting_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE spell_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
UPDATE milling_loot_template SET condition_id=(SELECT condition_entry FROM conditions WHERE type=lootcondition AND value1=condition_value1 AND value2=condition_value2 LIMIT 1) WHERE lootcondition>0;
