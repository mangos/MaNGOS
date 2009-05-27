alter table `creature_template`
    add column `unk1` int(11) UNSIGNED DEFAULT '0' NOT NULL after `heroic_entry`,
    add column `unk2` int(11) UNSIGNED DEFAULT '0' NOT NULL after `unk1`,
    add column `questItem1` int(11) UNSIGNED DEFAULT '0' NOT NULL after `RacialLeader`,
    add column `questItem2` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem1`,
    add column `questItem3` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem2`,
    add column `questItem4` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem3`,
    add column `movementId` int(11) UNSIGNED DEFAULT '0' NOT NULL after `questItem4`;
