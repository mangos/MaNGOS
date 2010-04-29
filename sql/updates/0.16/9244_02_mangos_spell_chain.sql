ALTER TABLE db_version CHANGE COLUMN required_9244_01_mangos_spell_proc_event required_9244_02_mangos_spell_chain bit;

INSERT INTO `spell_chain` VALUES
(49188, 0, 49188, 1, 0),
(56822, 49188, 49188, 2, 0),
(59057, 56822, 49188, 3, 0);
