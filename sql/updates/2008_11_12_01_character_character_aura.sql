ALTER TABLE character_db_version CHANGE COLUMN required_2008_11_07_03_characters_guild_bank_tab required_2008_11_12_01_character_character_aura bit;

ALTER TABLE `character_aura` ADD `stackcount` INT NOT NULL DEFAULT '1' AFTER `effect_index` ;
ALTER TABLE `pet_aura` ADD `stackcount` INT NOT NULL DEFAULT '1' AFTER `effect_index` ;

