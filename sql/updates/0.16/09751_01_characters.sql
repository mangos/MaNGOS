ALTER TABLE character_db_version CHANGE COLUMN required_9702_01_characters_item required_9751_01_characters bit;

ALTER TABLE `character_spell` ADD KEY `Idx_spell` (`spell`);
