ALTER TABLE db_version CHANGE COLUMN required_8589_02_mangos_gameobject_template required_8589_03_mangos_item_template bit;

alter table `item_template`
    add column `Faction` int(11) UNSIGNED DEFAULT '0' NOT NULL after `Flags`;
