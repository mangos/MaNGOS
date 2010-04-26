ALTER TABLE db_version CHANGE COLUMN required_9794_01_mangos_mangos_string required_9794_02_mangos_command bit;

DELETE FROM command WHERE name IN('account characters','account set addon','account set gmlevel','account set password');
INSERT INTO command (name, security, help) VALUES
('account characters',3,'Syntax: .account characters [#accountId|$accountName]\r\n\r\nShow list all characters for account seelcted by provided #accountId or $accountName, or for selected player in game.'),
('account set addon',3,'Syntax: .account set addon [#accountId|$accountName] #addon\r\n\r\nSet user (possible targeted) expansion addon level allowed. Addon values: 0 - normal, 1 - tbc, 2 - wotlk.'),
('account set gmlevel',4,'Syntax: .account set gmlevel [#accountId|$accountName] #level\r\n\r\nSet the security level for targeted player (can''t be used at self) or for #accountId or $accountName to a level of #level.\r\n\r\n#level may range from 0 to 3.'),
('account set password',4,'Syntax: .account set password (#accountId|$accountName) $password $password\r\n\r\nSet password for account.');
