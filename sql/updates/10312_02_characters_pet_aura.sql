ALTER TABLE character_db_version CHANGE COLUMN required_10312_01_characters_character_aura required_10312_02_characters_pet_aura bit;

ALTER TABLE `pet_aura` DROP PRIMARY KEY;
ALTER TABLE `pet_aura` ADD PRIMARY KEY (`guid`,`caster_guid`,`spell`);
