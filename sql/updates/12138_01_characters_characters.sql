ALTER TABLE character_db_version CHANGE COLUMN required_12112_02_characters_character required_12138_01_characters_characters bit;

ALTER TABLE `characters` DROP COLUMN `arenaPoints`;
ALTER TABLE `characters` DROP COLUMN `totalHonorPoints`;
ALTER TABLE `characters` DROP COLUMN `todayHonorPoints`;
ALTER TABLE `characters` DROP COLUMN `yesterdayHonorPoints`;
ALTER TABLE `characters` DROP COLUMN `knownCurrencies`;