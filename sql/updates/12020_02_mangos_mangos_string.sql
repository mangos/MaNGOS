ALTER TABLE db_version CHANGE COLUMN  required_12020_01_mangos_command  required_12020_02_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN (706, 727, 728, 729, 730, 731, 732, 737, 1005, 1015, 1022, 1023, 1024, 1025, 1029, 1100, 1503);
INSERT INTO mangos_string VALUES

(706,'This item(s) has problems with equipping/storing to inventory.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(727,'Your group has an offline member. Please remove them before joining.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(728,'Your group has players from the opposing faction. You cannot join the battleground as a group.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(729,'Your group has players from different battleground brakets. You cannot join as a group.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(730,'Someone in your party is already in this battleground queue. They must leave this queue before joining as a group.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(731,'Someone in your party is a Deserter. You cannot join as a group.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(732,'Someone in your party is already in three battleground queues. You cannot join as a group.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(737,'Arenas are set to 1v1 for debugging. You cannot join as a group.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1005,'Account name cannot be longer than 16 characters (client limit), account not created!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1015,'Can only quit from a Remote Admin console or the quit command was not entered in full (quit).',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1022,'ERROR: You can only assign a new name for a single selected character!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1023,'Character \'%s\' (GUID: %u Account %u) cannot be restored: account doesn\'t exist!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1024,'Character \'%s\' (GUID: %u Account %u) cannot be restored: account character list full!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1025,'Character \'%s\' (GUID: %u Account %u) cannot be restored: name already in use!',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1029,'Command can only be called from a Remote Admin console.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1100,'Account %s (Id: %u) has been granted %u expansion rights.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL),
(1503,'Cannot add spawn because no free guids for static spawn in reserved guids range. Server restart is required before command can be used. Also look GuidReserveSize.* config options.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

