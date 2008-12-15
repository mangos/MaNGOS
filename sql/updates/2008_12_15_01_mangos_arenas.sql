DELETE FROM `command` WHERE `name` = "flusharenapoints";
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('flusharenapoints','3','Syntax: .flusharenapoints\r\n\r\nUse it to distribute arena points based on arena team ratings, and start a new week.');

DELETE FROM mangos_string WHERE entry BETWEEN 7007 AND 7023;

INSERT INTO mangos_string (entry, content_default) VALUES
    (7007,'Your group is too large for this battleground. Please regroup to join.'),
    (7008,'Your group is too large for this arena. Please regroup to join.'),
    (7009,'Your group has members not in your arena team. Please regroup to join.'),
    (7010,'Your group does not have enough players to join this match.'),
    (7011,'The Gold Team wins!'),
    (7012,'The Green Team wins!'),
    (7013, 'There aren\'t enough players in this battleground. It will end soon unless some more players join to balance the fight.'),
    (7014, 'Your group has an offline member. Please remove him before joining.'),
    (7015, 'Your group has players from the opposing faction. You can\'t join the battleground as a group.'),
    (7016, 'Your group has players from different battleground brakets. You can\'t join as group.'),
    (7017, 'Someone in your party is already in this battleground queue. (S)he must leave it before joining as group.'),
    (7018, 'Someone in your party is Deserter. You can\'t join as group.'),
    (7019, 'Someone in your party is already in three battleground queues. You cannot join as group.'),
    (7020, 'You cannot teleport to a battleground or arena map.'),
    (7021, 'You cannot summon players to a battleground or arena map.'),
    (7022, 'You must be in GM mode to teleport to a player in a battleground.'),
    (7023, 'You cannot teleport to a battleground from another battleground. Please leave the current battleground first.');

DELETE FROM mangos_string WHERE entry = 714 OR entry = 716;
