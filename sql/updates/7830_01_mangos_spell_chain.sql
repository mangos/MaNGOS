ALTER TABLE db_version CHANGE COLUMN required_7823_01_mangos_item_template required_7830_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE  spell_id in (54424,57564,57565,57566,57567);

INSERT INTO `spell_chain` VALUES
 (54424,0,    54424,1,0),
 (57564,54424,54424,2,0),
 (57565,57564,54424,3,0),
 (57566,57565,54424,4,0),
 (57567,57566,54424,5,0);
