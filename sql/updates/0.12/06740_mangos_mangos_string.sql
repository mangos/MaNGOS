DELETE FROM mangos_string WHERE entry IN (332,333,334,335);
INSERT INTO mangos_string VALUES
(332,'GM mode is ON',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(333,'GM mode is OFF',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(334,'GM Chat Badge is ON',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(335,'GM Chat Badge is OFF',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

DELETE FROM mangos_string WHERE entry IN (713,714,715,716);
INSERT INTO mangos_string VALUES
(713,'You must be level %u to join an arena team!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(714,'%s is not high enough level to join your team',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(715,'You don\'t meet Battleground level requirements',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(716,'Your arena team is full, %s cannot join it.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);


DELETE FROM mangos_string WHERE entry IN (800,801,802,803,804,805,806,807,808,809);
INSERT INTO mangos_string VALUES
(800,'Invalid name',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(801,'You do not have enough gold',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(802,'You do not have enough free slots',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(803,'Your partner does not have enough free bag slots',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(804,'You do not have permission to perform that function',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(805,'Unknown language',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(806,'You don\'t know that language',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(807,'Please provide character name',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(808,'Player %s not found or offline',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(809,'Account for character %s not found',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
