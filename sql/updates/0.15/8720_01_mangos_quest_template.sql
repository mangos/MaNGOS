ALTER TABLE db_version CHANGE COLUMN required_8693_01_mangos_spell_proc_event required_8720_01_mangos_quest_template bit;

alter table `quest_template`
    add column `ReqItemId5` mediumint(8) UNSIGNED DEFAULT '0' NOT NULL after `ReqItemId4`,
    add column `ReqItemId6` mediumint(8) UNSIGNED DEFAULT '0' NOT NULL after `ReqItemId5`,
    add column `ReqItemCount5` smallint(5) UNSIGNED DEFAULT '0' NOT NULL after `ReqItemCount4`,
    add column `ReqItemCount6` smallint(5) UNSIGNED DEFAULT '0' NOT NULL after `ReqItemCount5`;
