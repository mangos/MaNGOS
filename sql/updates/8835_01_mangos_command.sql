ALTER TABLE db_version CHANGE COLUMN required_8833_02_mangos_command required_8835_01_mangos_command bit;

DELETE FROM command where name IN ('lookup title');

INSERT INTO `command` VALUES
('lookup title',2,'Syntax: .lookup title $$namepart\r\n\r\nLooks up a title by $namepart, and returns all matches with their title ID\'s and index\'s.');
