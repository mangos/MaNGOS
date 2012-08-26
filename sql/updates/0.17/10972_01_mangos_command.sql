ALTER TABLE db_version CHANGE COLUMN required_10951_01_mangos_spell_proc_event required_10972_01_mangos_command bit;

DELETE FROM command WHERE name IN ('send mass items','send mass mail','send mass money');

INSERT INTO command (name, security, help) VALUES
('send mass items',3,'Syntax: .send mass items #racemask|$racename|alliance|horde|all "#subject" "#text" itemid1[:count1] itemid2[:count2] ... itemidN[:countN]\r\n\r\nSend a mail to players. Subject and mail text must be in "". If for itemid not provided related count values then expected 1, if count > max items in stack then items will be send in required amount stacks. All stacks amount in mail limited to 12.'),
('send mass mail',1,'Syntax: .send mass mail #racemask|$racename|alliance|horde|all "#subject" "#text"\r\n\r\nSend a mail to players. Subject and mail text must be in "".'),
('send mass money','3','Syntax: .send mass money #racemask|$racename|alliance|horde|all "#subject" "#text" #money\r\n\r\nSend mail with money to players. Subject and mail text must be in "".');
