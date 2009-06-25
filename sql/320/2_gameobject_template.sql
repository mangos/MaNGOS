alter table `gameobject_template`
    add column `questItem5` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem4`,
    add column `questItem6` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem5`;
