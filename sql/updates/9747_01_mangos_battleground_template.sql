ALTER TABLE db_version CHANGE COLUMN required_9735_02_mangos_spell_chain required_9747_01_mangos_battleground_template bit;

UPDATE battleground_template
  SET MinPlayersPerTeam=5, MaxPlayersPerTeam=5 WHERE id IN (4,5,6,8,32);
