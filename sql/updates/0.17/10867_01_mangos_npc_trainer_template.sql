ALTER TABLE db_version CHANGE COLUMN required_10864_01_mangos_spell_proc_event required_10867_01_mangos_npc_trainer_template bit;

DROP TABLE IF EXISTS `npc_trainer_template`;
CREATE TABLE `npc_trainer_template` (
  `entry` mediumint(8) unsigned NOT NULL default '0',
  `spell` mediumint(8) unsigned NOT NULL default '0',
  `spellcost` int(10) unsigned NOT NULL default '0',
  `reqskill` smallint(5) unsigned NOT NULL default '0',
  `reqskillvalue` smallint(5) unsigned NOT NULL default '0',
  `reqlevel` tinyint(3) unsigned NOT NULL default '0',
  UNIQUE KEY `entry_spell` (`entry`,`spell`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
