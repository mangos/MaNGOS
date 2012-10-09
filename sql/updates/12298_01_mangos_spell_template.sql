ALTER TABLE db_version CHANGE COLUMN required_12275_01_mangos_creature_template_spells required_12298_01_mangos_spell_template bit;

DROP TABLE IF EXISTS `spell_template`;
