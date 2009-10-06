ALTER TABLE character_db_version CHANGE COLUMN required_8589_04_characters_groups required_8589_06_characters_bugreport bit;

alter table `characters`.`bugreport`
    change `type` `type` longtext NOT NULL,
    change `content` `content` longtext NOT NULL;
