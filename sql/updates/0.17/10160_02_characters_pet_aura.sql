ALTER TABLE character_db_version CHANGE COLUMN required_10160_01_characters_character_aura required_10160_02_characters_pet_aura bit;

alter table `pet_aura` drop primary key;
alter table `pet_aura` add primary key (`guid`,`spell`);