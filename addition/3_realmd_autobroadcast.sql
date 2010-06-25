CREATE TABLE IF NOT EXISTS `autobroadcast` (
  `id` int(11) NOT NULL auto_increment,
  `text` longtext NOT NULL,
  `next` int(11) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
