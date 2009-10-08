ALTER TABLE db_version CHANGE COLUMN required_8589_10_mangos_spell_proc_event required_8600_01_mangos_command bit;

DELETE FROM command where name='instance unbind';

INSERT INTO `command` VALUES
('instance unbind',3,'Syntax: .instance unbind all\r\n  All of the selected
player\'s binds will be cleared.\r\n.instance unbind #mapid\r\n Only the
specified #mapid instance will be cleared.');
