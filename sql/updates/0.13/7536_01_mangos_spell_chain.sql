ALTER TABLE db_version CHANGE COLUMN required_7503_01_mangos_command required_7536_01_mangos_spell_chain bit;

DELETE FROM spell_chain WHERE  spell_id in (13819,23214,34769,34767);

INSERT INTO `spell_chain` VALUES
 (13819,0,    13819,1,0    ),
 (23214,13819,13819,2,33391),
 (34769,0,    34769,1,0    ),
 (34767,34769,34769,2,33391);
