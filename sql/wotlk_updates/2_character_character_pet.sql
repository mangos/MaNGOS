alter table `character_pet`
    drop column `trainpoint`,
    drop column `loyaltypoints`,
    drop column `loyalty`,
    add `talentpoints` int(11) UNSIGNED default '0' NOT NULL after `Reactstate`;
