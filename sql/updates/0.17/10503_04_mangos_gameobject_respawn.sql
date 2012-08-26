ALTER TABLE db_version CHANGE COLUMN required_10503_03_mangos_creature_respawn required_10503_04_mangos_gameobject_respawn bit;

DROP TABLE IF EXISTS `gameobject_respawn`;
