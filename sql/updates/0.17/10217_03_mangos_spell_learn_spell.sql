ALTER TABLE db_version CHANGE COLUMN required_10217_02_mangos_playercreateinfo_action required_10217_03_mangos_spell_learn_spell bit;

-- 21084 replace of 20154 at learn judgements
DELETE FROM spell_learn_spell WHERE SpellID = 21084;
INSERT INTO spell_learn_spell VALUES
(20271,21084,1),
(53407,21084,1),
(53408,21084,1);
