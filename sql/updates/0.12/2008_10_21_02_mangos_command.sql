DELETE FROM command WHERE name IN (
  'acct','account','account create','account delete','account onlinelist',
  'account set addon','account set gmlevel','account set password',
  'chardelete','gm list','gm online','sendmessage','server corpses','server exit','server motd',
  'server set loglevel','server set motd','security'
);
INSERT INTO `command` VALUES
('account',0,'Syntax: .account\r\n\r\nDisplay the access level of your account.'),
('account create',4,'Syntax: .account create $account $password\r\n\r\nCreate account and set password to it.'),
('account delete',4,'Syntax: .account delete $account\r\n\r\nDelete account with all characters.'),
('account onlinelist',4,'Syntax: .account onlinelist\r\n\r\nShow list of online accounts.'),
('account set addon',3,'Syntax: .account set addon [$account] #addon\r\n\r\nSet user (posible targeted) expansion addon level allowed. Addon values: 0 - normal, 1 - tbc, 2 - wotlk.'),
('account set gmlevel',4,'Syntax: .account set gmlevel [$account] #level\r\n\r\nSet the security level for targeted player (can''t be used at self) or for account $name to a level of #level.\r\n\r\n#level may range from 0 to 3.'),
('account set password',4,'Syntax: .account set password $account $password $password\r\n\r\nSet password for account.'),
('chardelete',4,'Syntax: .chardelete $charactername\r\n\r\nDelete character.'),
('gm list',3,'Syntax: .gm list\r\n\r\nDisplay a list of all Game Masters accounts and security levels.'),
('gm online',0,'Syntax: .gm online\r\n\r\nDisplay a list of available Game Masters.'),
('sendmessage',3,'Syntax: .sendmessage $playername $message\r\n\r\nSend screen message to player from ADMINISTRATOR.'),
('server corpses',2,'Syntax: .server corpses\r\n\r\nTriggering corpses expire check in world.'),
('server exit',4,'Syntax: .server exit\r\n\r\nTerminate mangosd NOW.'),
('server motd',0,'Syntax: .server motd\r\n\r\nShow server Message of the day.'),
('server set loglevel',4,'Syntax: .server set loglevel #level\r\n\r\nSet server log level (0 - errors only, 1 - basic, 2 - detail, 3 - debug).'),
('server set motd',3,'Syntax: .server set motd $MOTD\r\n\r\nSet server Message of the day.');
