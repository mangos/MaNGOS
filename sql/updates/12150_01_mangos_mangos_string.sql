ALTER TABLE db_version CHANGE COLUMN required_12141_02_mangos_mangos_command required_12150_01_mangos_mangos_string bit;

DELETE FROM `mangos_string` WHERE `entry` BETWEEN 741 AND 746;
