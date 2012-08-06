ALTER TABLE character_db_version CHANGE COLUMN required_10156_01_characters_character_aura required_10156_02_characters_pet_aura bit;

DELETE FROM `pet_aura`;
ALTER TABLE `pet_aura`
    DROP COLUMN `effect_index`, DROP COLUMN `amount`, DROP COLUMN `maxduration`, DROP COLUMN `remaintime`;
ALTER TABLE `pet_aura`
    ADD COLUMN `basepoints0` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `basepoints1` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `basepoints2` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `maxduration0` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `maxduration1` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `maxduration2` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `remaintime0` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `remaintime1` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `remaintime2` INT(11) NOT NULL DEFAULT '0',
    ADD COLUMN `effIndexMask` INT(11) NOT NULL DEFAULT '0';