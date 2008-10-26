alter table `item_template`
    add column `StatsCount` tinyint(3) UNSIGNED DEFAULT '0' NOT NULL after `ContainerSlots`;
