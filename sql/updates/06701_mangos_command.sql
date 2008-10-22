DELETE FROM command WHERE name IN ('lookup player account','lookup player ip','lookup player email');
INSERT INTO `command` VALUES ('lookup player account',2,'Syntax : .lookup player account $account ($limit) \r\n\r\n Searchs players, which account username is $account with optional parametr $limit of results.');
INSERT INTO `command` VALUES ('lookup player ip',2,'Syntax : .lookup player ip $ip ($limit) \r\n\r\n Searchs players, which account ast_ip is $ip with optional parametr $limit of results.');
INSERT INTO `command` VALUES ('lookup player email',2,'Syntax : .lookup player email $email ($limit) \r\n\r\n Searchs players, which account email is $email with optional parametr $limit of results.');
