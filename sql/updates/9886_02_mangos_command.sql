ALTER TABLE db_version CHANGE COLUMN required_9886_01_mangos_mangos_string required_9886_02_mangos_command bit;

DELETE FROM command WHERE name IN('lookup account email','lookup account ip','lookup account name','lookup player account','lookup player ip','lookup player email');
INSERT INTO command (name, security, help) VALUES
('lookup account email',2,'Syntax: .lookup account email $emailpart [#limit] \r\n\r\n Searchs accounts, which email including $emailpart with optional parametr #limit of results. If #limit not provided expected 100.'),
('lookup account ip',2,'Syntax: lookup account ip $ippart [#limit] \r\n\r\n Searchs accounts, which last used ip inluding $ippart (textual) with optional parametr #$limit of results. If #limit not provided expected 100.'),
('lookup account name',2,'Syntax: .lookup account name $accountpart [#limit] \r\n\r\n Searchs accounts, which username including $accountpart with optional parametr #limit of results. If #limit not provided expected 100.'),
('lookup player account',2,'Syntax: .lookup player account $accountpart [#limit] \r\n\r\n Searchs players, which account username including $accountpart with optional parametr #limit of results. If #limit not provided expected 100.'),
('lookup player email',2,'Syntax: .lookup player email $emailpart [#limit] \r\n\r\n Searchs players, which account email including $emailpart with optional parametr #limit of results. If #limit not provided expected 100.'),
('lookup player ip',2,'Syntax: .lookup player ip $ippart [#limit] \r\n\r\n Searchs players, which account last used ip inluding $ippart (textual) with optional parametr #limit of results. If #limit not provided expected 100.');
