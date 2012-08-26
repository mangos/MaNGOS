ALTER TABLE character_db_version CHANGE COLUMN required_10156_02_characters_pet_aura required_10160_01_characters_character_aura bit;

alter table `character_aura` drop primary key;
alter table `character_aura` add primary key (`guid`,`spell`);