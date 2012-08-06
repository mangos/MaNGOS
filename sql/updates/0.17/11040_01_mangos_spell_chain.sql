ALTER TABLE db_version CHANGE COLUMN required_11036_01_mangos_spell_threat required_11040_01_mangos_spell_chain bit;

DELETE FROM `spell_chain` WHERE `first_spell` IN (5672);

INSERT INTO `spell_chain` VALUES
/* Healing Stream Totem Aura */
(5672,0,5672,1,0),
(6371,5672,5672,2,0),
(6372,6371,5672,3,0),
(10460,6372,5672,4,0),
(10461,10460,5672,5,0),
(25566,10461,5672,6,0),
(58763,25566,5672,7,0),
(58764,58763,5672,8,0),
(58765,58764,5672,9,0);
