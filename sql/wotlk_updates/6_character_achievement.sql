CREATE TABLE IF NOT EXISTS `character_achievement` (
      `guid` int(11) NOT NULL,
      `achievement` int(11) NOT NULL,
      `date` int(11) NOT NULL,
      PRIMARY KEY  (`guid`,`achievement`)
    ) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `character_achievement_progress` (
      `guid` int(11) NOT NULL,
      `criteria` int(11) NOT NULL,
      `counter` int(11) NOT NULL,
      `date` int(11) NOT NULL,
      PRIMARY KEY  (`guid`,`criteria`)
    ) ENGINE=MyISAM DEFAULT CHARSET=utf8;
