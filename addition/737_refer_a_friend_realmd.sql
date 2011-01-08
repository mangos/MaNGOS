-- Refer-a-friend system by MasOn
-- table by ghostpast
CREATE TABLE IF NOT EXISTS `account_friends` 
(
  `id` int(11) unsigned NOT NULL default '0',
  `friend_id` int(11) unsigned NOT NULL default '0',
  `bind_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Bring date',
  `expire_date` datetime NOT NULL DEFAULT 0 COMMENT 'Expire date',
   PRIMARY KEY  (`id`, `friend_id`)
)
DEFAULT CHARSET=utf8 PACK_KEYS=0 COMMENT='Stores accounts for refer-a-friend system.';
