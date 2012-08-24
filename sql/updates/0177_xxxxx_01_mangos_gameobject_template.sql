ALTER TABLE db_version CHANGE COLUMN required_0176_xxxxx_01_mangos_gameobject_template required_0177_xxxxx_01_mangos_gameobject_template bit;

ALTER TABLE `gameobject_template` MODIFY COLUMN `data24` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data23`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `data25` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data24`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `data26` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data25`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `data27` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data26`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `data28` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data27`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `data29` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data28`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `data30` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data29`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `data31` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data30`;
ALTER TABLE `gameobject_template` MODIFY COLUMN `unk2` int(10) unsigned NOT NULL DEFAULT '0' AFTER `data31`;