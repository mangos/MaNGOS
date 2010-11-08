ALTER TABLE db_version CHANGE COLUMN required_10682_01_mangos_item_convert required_10704_01_mangos_gossip_menu_option bit;

ALTER TABLE gossip_menu_option CHANGE COLUMN action_menu_id action_menu_id MEDIUMINT(8) NOT NULL DEFAULT '0';
