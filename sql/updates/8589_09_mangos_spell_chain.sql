ALTER TABLE db_version CHANGE COLUMN required_8589_08_mangos_item_template required_8589_09_mangos_spell_chain bit;

/* UnholyBlight non ranked now */
DELETE FROM spell_chain WHERE first_spell = 49194;
