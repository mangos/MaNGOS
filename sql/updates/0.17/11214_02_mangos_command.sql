ALTER TABLE db_version CHANGE COLUMN required_11214_01_mangos_mangos_string required_11214_02_mangos_command bit;

DELETE FROM command WHERE name = 'debug spellcoefs';

INSERT INTO command (name, security, help) VALUES
('debug spellcoefs',3,'Syntax: .debug spellcoefs #pellid\r\n\r\nShow default calculated and DB stored coefficients for direct/dot heal/damage.');
