ALTER TABLE db_version CHANGE COLUMN required_8818_01_mangos_mangos_string required_8828_02_mangos_instance_template bit;

ALTER TABLE instance_template
  DROP COLUMN maxPlayers,
  DROP COLUMN maxPlayersHeroic,
  DROP COLUMN reset_delay;
