alter table `item_template`
    add column `Faction` int(11) UNSIGNED DEFAULT '0' NOT NULL after `Flags`;
