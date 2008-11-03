alter table `quest_template`
    add column `PlayersSlain` tinyint(3) UNSIGNED DEFAULT '0' NOT NULL after `CharTitleId`,
    add column `BonusTalents` tinyint(3) UNSIGNED DEFAULT '0' NOT NULL after `PlayersSlain`;
