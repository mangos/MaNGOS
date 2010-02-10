ALTER TABLE db_version CHANGE COLUMN required_8883_02_mangos_spell_bonus_data required_8886_01_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN(60,61,62);
INSERT INTO mangos_string VALUES
(60,'I\'m busy right now, come back later.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(61,'Username: ',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(62,'Password: ',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
