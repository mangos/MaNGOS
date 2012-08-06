ALTER TABLE character_db_version CHANGE COLUMN required_10332_01_characters_character_aura required_10332_02_characters_pet_aura bit;

ALTER TABLE `pet_aura`
  ADD COLUMN `item_guid` int(11) unsigned NOT NULL default '0' AFTER `caster_guid`,
  DROP PRIMARY KEY,
  ADD PRIMARY KEY (`guid`,`caster_guid`,`item_guid`,`spell`);

