ALTER TABLE db_version CHANGE COLUMN required_8589_01_mangos_creature_template required_8589_02_mangos_gameobject_template bit;

alter table `gameobject_template`
    add column `questItem5` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem4`,
    add column `questItem6` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem5`;
