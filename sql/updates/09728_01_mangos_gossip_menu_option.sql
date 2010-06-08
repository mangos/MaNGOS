ALTER TABLE db_version CHANGE COLUMN required_9720_01_mangos_spell_proc_event required_9728_01_mangos_gossip_menu_option bit;

UPDATE gossip_menu_option SET option_icon=0 WHERE menu_id=0 AND option_id=16;
