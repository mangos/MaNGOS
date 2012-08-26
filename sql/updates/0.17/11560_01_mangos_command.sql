ALTER TABLE db_version CHANGE COLUMN required_11549_01_mangos_spell_proc_event required_11560_01_mangos_command bit;

DELETE FROM command WHERE name = 'mailbox';

INSERT INTO command (name, security, help) VALUES
('mailbox',3,'Syntax: .mailbox\r\n\r\nShow your mailbox content.');
