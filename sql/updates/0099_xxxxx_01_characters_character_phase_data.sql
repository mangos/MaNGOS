ALTER TABLE character_db_version CHANGE COLUMN required_0096_xxxxx_01_characters_characters required_0099_xxxxx_01_characters_character_phase_data bit;

ALTER TABLE `character_phase_data` CHANGE COLUMN `phase` `phase` int(11) NOT NULL DEFAULT 0 COMMENT '';