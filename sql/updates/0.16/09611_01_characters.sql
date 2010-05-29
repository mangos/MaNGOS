ALTER TABLE character_db_version CHANGE COLUMN required_9375_01_characters_character_glyphs required_9611_01_characters bit;

ALTER TABLE `group_member`            ADD INDEX `Idx_memberGuid`(`memberGuid`);
ALTER TABLE `guild_eventlog`          ADD INDEX `Idx_PlayerGuid1`(`PlayerGuid1`);
ALTER TABLE `guild_eventlog`          ADD INDEX `Idx_PlayerGuid2`(`PlayerGuid2`);
ALTER TABLE `guild_bank_eventlog`     ADD INDEX `Idx_PlayerGuid`(`PlayerGuid`);
ALTER TABLE `petition_sign`           ADD INDEX `Idx_playerguid`(`playerguid`);
ALTER TABLE `petition_sign`           ADD INDEX `Idx_ownerguid`(`ownerguid`);
ALTER TABLE `guild_eventlog`          ADD INDEX `Idx_LogGuid`(`LogGuid`);
ALTER TABLE `guild_bank_eventlog`     ADD INDEX `Idx_LogGuid`(`LogGuid`);
ALTER TABLE `guild_bank_item`         ADD INDEX `Idx_item_guid`(`item_guid`);
ALTER TABLE `corpse`                  ADD INDEX `Idx_player`(`player`);
ALTER TABLE `corpse`                  ADD INDEX `Idx_time`(`time`);
ALTER TABLE `guild_rank`              ADD INDEX `Idx_rid`(`rid`);
ALTER TABLE `character_equipmentsets` ADD INDEX `Idx_setindex` (`setindex`);
