ALTER TABLE db_version CHANGE COLUMN required_11523_02_mangos_mangos_string required_11529_01_mangos_command bit;

DELETE FROM command WHERE name = 'gobject turn';

INSERT INTO command (name, security, help) VALUES
('gobject turn',2,'Syntax: .gobject turn #goguid [#z_angle]\r\n\r\nChanges gameobject #goguid orientation (rotates gameobject around z axis). Optional parameters are (#y_angle,#x_angle) values that represents rotation angles around y and x axes.');
