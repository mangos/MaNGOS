ALTER TABLE character_db_version CHANGE COLUMN required_11117_02_characters_world required_11299_01_characters_character_aura bit;

TRUNCATE TABLE character_aura;
ALTER TABLE character_aura
  CHANGE COLUMN `maxduration0` `maxduration` INT(11) NOT NULL DEFAULT '0',
  CHANGE COLUMN `remaintime0`  `remaintime`  INT(11) NOT NULL DEFAULT '0',
  ADD COLUMN `periodictime0` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `basepoints2`,
  ADD COLUMN `periodictime1` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `periodictime0`,
  ADD COLUMN `periodictime2` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `periodictime1`,
  DROP COLUMN `maxduration1`,
  DROP COLUMN `maxduration2`,
  DROP COLUMN `remaintime1`,
  DROP COLUMN `remaintime2`,
  CHANGE COLUMN `stackcount` `stackcount` INT(11) UNSIGNED NOT NULL DEFAULT '1',
  CHANGE COLUMN `remaincharges` `remaincharges` INT(11) UNSIGNED NOT NULL DEFAULT '0',
  CHANGE COLUMN `effIndexMask` `effIndexMask` INT(11) UNSIGNED NOT NULL DEFAULT '0';
