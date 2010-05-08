alter table `characters`.`characters`
    add column `power8` int(10) UNSIGNED DEFAULT '0' NOT NULL after `power7`,
    add column `power9` int(10) UNSIGNED DEFAULT '0' NOT NULL after `power8`;

alter table `characters`.`character_stats`
    add column `maxpower8` int(10) UNSIGNED DEFAULT '0' NOT NULL after `maxpower7`,
    add column `maxpower9` int(10) UNSIGNED DEFAULT '0' NOT NULL after `maxpower8`;
