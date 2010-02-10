ALTER TABLE db_version CHANGE COLUMN required_7988_04_mangos_creature_template required_7988_05_mangos_item_template bit;

alter table `item_template`
    add column `HolidayId` int(11) UNSIGNED DEFAULT '0' NOT NULL after `ItemLimitCategory`;
