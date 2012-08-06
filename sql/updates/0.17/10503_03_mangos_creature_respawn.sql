ALTER TABLE db_version CHANGE COLUMN required_10500_01_mangos_scripts required_10503_03_mangos_creature_respawn bit;

DROP TABLE IF EXISTS `creature_respawn`;
