ALTER TABLE character_db_version CHANGE COLUMN required_2008_11_07_01_characters_character_db_version required_2008_11_07_03_characters_guild_bank_tab bit;

ALTER TABLE `guild_bank_tab`
  CHANGE COLUMN `TabText` `TabText` text;
