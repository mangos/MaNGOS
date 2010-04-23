ALTER TABLE db_version CHANGE COLUMN required_7643_02_mangos_mangos_string required_7662_01_mangos_spell_chain bit;

DELETE FROM `spell_chain` WHERE spell_id IN (50288, 53191, 53194, 53195);
INSERT INTO `spell_chain` VALUES
(50288, 0, 50288, 1, 0),
(53191, 50288, 50288, 2, 0),
(53194, 53191, 50288, 3, 0),
(53195, 53194, 50288, 4, 0);

DELETE FROM `spell_chain` WHERE spell_id IN (50294, 53188, 53189, 53190);
INSERT INTO `spell_chain` VALUES
(50294, 0, 50294, 1, 0),
(53188, 50294, 50294, 2, 0),
(53189, 53188, 50294, 3, 0),
(53190, 53189, 50294, 4, 0);
