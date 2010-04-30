ALTER TABLE db_version CHANGE COLUMN required_9160_02_mangos_spell_chain required_9198_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry in (1132,1133,1134);

INSERT INTO mangos_string VALUES
(1132,'   Follow player %s (lowguid %u)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1133,'   Follow creature %s (lowguid %u)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1134,'   Follow <NULL>',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
