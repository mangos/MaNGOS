alter table `characters`
    add column `power8` int(10) UNSIGNED DEFAULT '0' NOT NULL after `power7`,
    add column `power9` int(10) UNSIGNED DEFAULT '0' NOT NULL after `power8`,
    add column `power10` int(10) UNSIGNED DEFAULT '0' NOT NULL after `power9`,
    drop column `ammoId`;

alter table `character_stats`
    add column `maxpower8` int(10) UNSIGNED DEFAULT '0' NOT NULL after `maxpower7`,
    add column `maxpower9` int(10) UNSIGNED DEFAULT '0' NOT NULL after `maxpower8`,
    add column `maxpower10` int(10) UNSIGNED DEFAULT '0' NOT NULL after `maxpower9`;
