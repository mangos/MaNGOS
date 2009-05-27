alter table `gameobject_template`
    add column `unk1` varchar(100) NOT NULL default ''            after `castBarCaption`,
    add column `questItem1` int(11) UNSIGNED DEFAULT '0' NOT NULL after `size`,
    add column `questItem2` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem1`,
    add column `questItem3` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem2`,
    add column `questItem4` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem3`;
