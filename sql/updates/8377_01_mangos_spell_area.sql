ALTER TABLE db_version CHANGE COLUMN required_8364_01_mangos_db_version required_8377_01_mangos_spell_area bit;

DELETE FROM `spell_area` where spell in (40216,42016);
