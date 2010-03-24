ALTER TABLE db_version CHANGE COLUMN required_9589_01_mangos_creature_template required_9590_01_mangos_db_script_string bit;

ALTER TABLE `db_script_string` CHANGE `entry` `entry` INT( 11 ) UNSIGNED NOT NULL DEFAULT '0';
