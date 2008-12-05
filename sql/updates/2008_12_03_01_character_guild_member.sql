ALTER TABLE character_db_version CHANGE COLUMN required_2008_11_12_01_character_character_aura required_2008_12_03_01_character_guild_member bit;
ALTER TABLE `guild_member` DROP INDEX `guid_key` ,
ADD UNIQUE `guid_key` ( `guid` );
