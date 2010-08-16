ALTER TABLE db_version CHANGE COLUMN required_10323_02_mangos_command required_10331_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (131,132,562,563,564,565,566,567,568,569,570,571,572,575,576,1153,1154,1155,1156,1157,1158,1159,1160);

INSERT INTO mangos_string VALUES
(131,'You changed the %s spellmod %u to value %i for spell with family bit %u for %s.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(132,'%s changed your spellmod %u to value %i for spell with family bit %u.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(564,'The value index %u is too big to %s (count: %u).',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(565,'Set for %s field:%u to uint32 value:%u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(566,'You set for %s field:%u to uint32 value: %u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(567,'Set for %s field:%u to to float value:%f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(568,'You set for %s field:%u to float value: %f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(569,'Get %s uint32 value:[FIELD]:%u [VALUE]:%u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(570,'%s has uint32 value:[FIELD]:%u [VALUE]:%u',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(571,'Get %s float value:[FIELD]:%u [VALUE]:%f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(572,'%s has float value:[FIELD]:%u [VALUE]:%f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(575,'Modify %s uint32 field:%u to sum with:%i = %u (%i)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(576,'You modify for %s uint32 field:%u to sum with:%i = %u (%i)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1153,'Get %s bitstr value:[FIELD]:%u [VALUE]:%s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1154,'%s has bitstr value:[FIELD]:%u [VALUE]:%s',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1155,'Get %s hex value:[FIELD]:%u [VALUE]:%x',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1156,'%s has hex value:[FIELD]:%u [VALUE]:%x',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1157,'Modify %s hex field:%u %s %x = %x (hex)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1158,'You modify for %s hex field:%u %s %x = %x (hex)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1159,'Modify %s float field:%u to sum with:%f = %f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1160,'You modify for %s float field:%u to sum with:%f = %f',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
