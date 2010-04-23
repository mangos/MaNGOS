ALTER TABLE db_version CHANGE COLUMN required_7988_06_mangos_gameobject_template required_7988_08_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` IN (18265);
