ALTER TABLE character_db_version CHANGE COLUMN required_8505_01_characters_character_spell required_8589_04_characters_groups bit;

alter table `groups`
    add column `raiddifficulty` int(11) UNSIGNED DEFAULT '0' NOT NULL after `difficulty`;
