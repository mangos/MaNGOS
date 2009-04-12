ALTER TABLE db_version CHANGE COLUMN required_2008_11_09_01_mangos_command required_2008_11_09_02_mangos_command bit;

delete from `command` where `name` = 'sendmoney';
insert into `command` (`name`, `security`, `help`) values
('sendmoney',3,'Syntax: .sendmoney #playername "#subject" "#text" #money\r\n\r\nSend mail with money to a player. Subject and mail text must be in "".');
