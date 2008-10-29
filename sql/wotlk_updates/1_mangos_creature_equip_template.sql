TRUNCATE creature_equip_template;

alter table `creature_equip_template`
    drop column `equipinfo1`,
    drop column `equipinfo2`,
    drop column `equipinfo3`,
    drop column `equipslot1`,
    drop column `equipslot2`,
    drop column `equipslot3`,
    change `equipmodel1` `equipentry1` mediumint(8) UNSIGNED default '0' NOT NULL,
    change `equipmodel2` `equipentry2` mediumint(8) UNSIGNED default '0' NOT NULL,
    change `equipmodel3` `equipentry3` mediumint(8) UNSIGNED default '0' NOT NULL;
