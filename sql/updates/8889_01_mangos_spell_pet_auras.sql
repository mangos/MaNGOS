ALTER TABLE db_version CHANGE COLUMN required_8886_01_mangos_string required_8889_01_mangos_spell_pet_auras bit;

DELETE FROM spell_pet_auras WHERE aura = 57989;

INSERT INTO `spell_pet_auras` VALUES
(58228, 0, 19668, 57989);