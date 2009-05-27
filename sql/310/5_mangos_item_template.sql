alter table `item_template`
    add column `HolidayId` int(11) UNSIGNED DEFAULT '0' NOT NULL after `ItemLimitCategory`;
