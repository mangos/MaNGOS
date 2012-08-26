ALTER TABLE db_version CHANGE COLUMN required_10203_01_mangos_item_template required_10205_01_mangos_spell_area bit;

DELETE FROM spell_area WHERE spell = 58600;
