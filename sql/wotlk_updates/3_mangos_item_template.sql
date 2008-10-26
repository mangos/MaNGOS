alter table `item_template`
    add column `ScalingStatDistribution` smallint(6) DEFAULT '0' NOT NULL after `stat_value10`,
    add column `ScalingStatValue` smallint(6) DEFAULT '0' NOT NULL after `ScalingStatDistribution`,
    add column `ItemLimitCategory` smallint(6) DEFAULT '0' NOT NULL after `ArmorDamageModifier`,
    change `Duration` `Duration` int(11) NOT NULL default '0' COMMENT 'Duration in seconds. Negative value means realtime, postive value ingame time' after ArmorDamageModifier;
