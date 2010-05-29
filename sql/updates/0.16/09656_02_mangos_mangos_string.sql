ALTER TABLE db_version CHANGE COLUMN required_9656_01_mangos_command required_9656_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry in (1135,1136);

INSERT INTO mangos_string VALUES
(1135,'List known talents:',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1136,'   (Found talents: %u used talent points: %u)',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
