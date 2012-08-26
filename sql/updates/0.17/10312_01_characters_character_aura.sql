ALTER TABLE character_db_version CHANGE COLUMN required_10254_01_characters_auctionhouse required_10312_01_characters_character_aura bit;

ALTER TABLE `character_aura` DROP PRIMARY KEY;
ALTER TABLE `character_aura` ADD PRIMARY KEY (`guid`,`caster_guid`,`spell`);
