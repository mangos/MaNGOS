ALTER TABLE db_version CHANGE COLUMN required_10400_01_mangos_mangos_string required_10409_01_mangos_spell_chain bit;

INSERT INTO spell_chain VALUES
(47230, 0, 47230, 1, 0),
(47231, 47230, 47230, 2, 0);
