ALTER TABLE db_version CHANGE COLUMN required_12012_01_mangos_spell_template required_12087_01_mangos_mangos_string bit;

UPDATE mangos_string SET content_default='This item(s) has problems with equipping/storing to inventory.' WHERE entry=706;
UPDATE mangos_string SET content_default='Arenas are set to 1v1 for debugging. You cannot join as a group.' WHERE entry=737;
UPDATE mangos_string SET content_default='Account name cannot be longer than 16 characters (client limit), account not created!' WHERE entry=1005;
UPDATE mangos_string SET content_default='Can only quit from a Remote Admin console or the quit command was not entered in full (quit).' WHERE entry=1015;
UPDATE mangos_string SET content_default='ERROR: You can only assign a new name for a single selected character!' WHERE entry=1022;
UPDATE mangos_string SET content_default='Character \'%s\' (GUID: %u Account %u) can\'t be restored: account doesn\'t exist!' WHERE entry=1023;
UPDATE mangos_string SET content_default='Character \'%s\' (GUID: %u Account %u) can\'t be restored: name already in use!' WHERE entry=1025;
UPDATE mangos_string SET content_default='Command can only be called from a Remote Admin console.' WHERE entry=1029;
UPDATE mangos_string SET content_default='Account %s (Id: %u) has been granted %u expansion rights.' WHERE entry=1100;
UPDATE mangos_string SET content_default='Cannot add spawn because no free guids for static spawn in reserved guids range. Server restart is required before command can be used. Also look GuidReserveSize.* config options.' WHERE entry=1503;
