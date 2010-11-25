-- Anticheat tables from /dev/rsa
-- Config
DROP TABLE IF EXISTS `anticheat_config`;
CREATE TABLE IF NOT EXISTS `anticheat_config` (
  `checktype`          mediumint(8) unsigned NOT NULL COMMENT 'Type of check',
  `description`        varchar(255),
  `check_period`       int(11) unsigned NOT NULL default '0' COMMENT 'Time period of check, in ms, 0 - always',
  `alarmscount`        int(11) unsigned NOT NULL default '1'COMMENT 'Count of alarms before action',
  `disableoperation`   tinyint(3) unsigned NOT NULL default '0'COMMENT 'Anticheat disable operations in main core code after check fail',
  `messagenum`         int(11) NOT NULL default '0' COMMENT 'Number of system message',
  `intparam1`          mediumint(8) NOT NULL default '0' COMMENT 'Int parameter 1',
  `intparam2`          mediumint(8) NOT NULL default '0' COMMENT 'Int parameter 2',
  `floatparam1`        float NOT NULL default '0' COMMENT 'Float parameter 1',
  `floatparam2`        float NOT NULL default '0' COMMENT 'Float parameter 2',
  `action1`            mediumint(8) NOT NULL default '0' COMMENT 'Action 1',
  `actionparam1`       mediumint(8) NOT NULL default '0' COMMENT 'Action parameter 1',
  `action2`            mediumint(8) NOT NULL default '0' COMMENT 'Action 1',
  `actionparam2`       mediumint(8) NOT NULL default '0' COMMENT 'Action parameter 1',
  PRIMARY KEY  (`checktype`)
) DEFAULT CHARSET=utf8 PACK_KEYS=0 COMMENT='Anticheat configuration';

-- DROP TABLE IF EXISTS `anticheat_log`;
CREATE TABLE IF NOT EXISTS `anticheat_log` (
  `playername`     varchar(32) NOT NULL,
  `checktype`      mediumint(8) unsigned NOT NULL,
  `alarm_time`     datetime NOT NULL,
  `reason`         varchar(255) NOT NULL DEFAULT 'Unknown',
  `guid`           int(11) unsigned NOT NULL,
  `action1`        mediumint(8) NOT NULL default '0',
  `action2`        mediumint(8) NOT NULL default '0',
   PRIMARY KEY (`checktype`, `alarm_time`, `guid`),
   KEY `idx_Player` (`guid`)
)  DEFAULT CHARSET=utf8 COMMENT='Anticheat log table';

-- Configuration for anticheat
-- Main checks
REPLACE INTO `anticheat_config` 
(`checktype`, `description`, `check_period`, `alarmscount`, `disableoperation`,`messagenum`, `intparam1`, `intparam2`, `floatparam1`, `floatparam2`, `action1`, `actionparam1`, `action2`, `actionparam2`) VALUES
(0, "Null check",         0, 1, 0, 11000, 0, 0,    0, 0, 1, 0, 0, 0),
(1, "Movement cheat",     0, 1, 0, 11000, 0, 0,    0, 0, 2, 1, 0, 0),
(2, "Spell cheat",        0, 1, 0, 11000, 0, 0,    0, 0, 2, 1, 0, 0),
(3, "Quest cheat",        0, 1, 0, 11000, 0, 0,    0, 0, 2, 1, 0, 0),
(4, "Transport cheat",    0, 3, 0, 11000, 0, 0, 60.0, 0, 2, 1, 0, 0),
(5, "Damage cheat",       0, 1, 0, 11000, 0, 0,    0, 0, 2, 1, 0, 0);

-- Subchecks
REPLACE INTO `anticheat_config` 
(`checktype`, `description`, `check_period`, `alarmscount`, `disableoperation`, `messagenum`, `intparam1`, `intparam2`, `floatparam1`, `floatparam2`, `action1`, `actionparam1`, `action2`, `actionparam2`) VALUES
(101, "Speed hack",              500, 5, 0, 11000,    10000, 0, 0.0012,    0, 2, 1, 6, 20000),
(102, "Fly hack",                500, 5, 0, 11000,    20000, 0,   10.0,    0, 2, 1, 0, 0),
(103, "Wall climb hack",         500, 2, 0, 11000,    10000, 0, 0.0015, 2.37, 2, 1, 0, 0),
(104, "Waterwalking hack",      1000, 3, 0, 11000,    20000, 0,      0,    0, 2, 1, 0, 0),
(105, "Teleport to plane hack",  500, 1, 0, 11000,        0, 0, 0.0001,  0.1, 2, 1, 0, 0),
(106, "AirJump hack" ,           500, 3, 0, 11000,    30000, 0,   10.0, 25.0, 2, 1, 0, 0),
(107, "Teleport hack" ,            0, 1, 0, 11000,        0, 0,   50.0,    0, 2, 1, 0, 0),
(108, "Fall hack" ,                0, 3, 0, 11000,    10000, 0,      0,    0, 2, 1, 0, 0),
--
(201, "Spell invalid",             0, 1, 0, 11000,        0, 0,      0,    0, 2, 1, 0, 0),
(202, "Spellcast in dead state",   0, 1, 0, 11000,        0, 0,      0,    0, 2, 1, 0, 0),
(203, "Spell not valid for player",0, 1, 0, 11000,        0, 0,      0,    0, 2, 1, 0, 0),
(204, "Spell not in player book",  0, 1, 0, 11000,        0, 0,      0,    0, 2, 1, 0, 0),
--
(501, "Spell damage hack",         0, 1, 0, 11000,        0, 50000,  0,    0, 2, 1, 0, 0),
(502, "Melee damage hack",         0, 1, 0, 11000,        0, 50000,  0,    0, 2, 1, 0, 0);
