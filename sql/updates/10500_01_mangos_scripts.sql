ALTER TABLE db_version CHANGE COLUMN required_10457_01_mangos_spell_proc_event required_10500_01_mangos_scripts bit;

ALTER TABLE creature_movement_scripts ADD COLUMN dataint2 int(11) NOT NULL default '0' AFTER dataint;
ALTER TABLE creature_movement_scripts ADD COLUMN dataint3 int(11) NOT NULL default '0' AFTER dataint2;
ALTER TABLE creature_movement_scripts ADD COLUMN dataint4 int(11) NOT NULL default '0' AFTER dataint3;
ALTER TABLE creature_movement_scripts ADD COLUMN comments varchar(255) NOT NULL AFTER o;

ALTER TABLE event_scripts ADD COLUMN dataint2 int(11) NOT NULL default '0' AFTER dataint;
ALTER TABLE event_scripts ADD COLUMN dataint3 int(11) NOT NULL default '0' AFTER dataint2;
ALTER TABLE event_scripts ADD COLUMN dataint4 int(11) NOT NULL default '0' AFTER dataint3;
ALTER TABLE event_scripts ADD COLUMN comments varchar(255) NOT NULL AFTER o;

ALTER TABLE gameobject_scripts ADD COLUMN dataint2 int(11) NOT NULL default '0' AFTER dataint;
ALTER TABLE gameobject_scripts ADD COLUMN dataint3 int(11) NOT NULL default '0' AFTER dataint2;
ALTER TABLE gameobject_scripts ADD COLUMN dataint4 int(11) NOT NULL default '0' AFTER dataint3;
ALTER TABLE gameobject_scripts ADD COLUMN comments varchar(255) NOT NULL AFTER o;

ALTER TABLE gossip_scripts ADD COLUMN dataint2 int(11) NOT NULL default '0' AFTER dataint;
ALTER TABLE gossip_scripts ADD COLUMN dataint3 int(11) NOT NULL default '0' AFTER dataint2;
ALTER TABLE gossip_scripts ADD COLUMN dataint4 int(11) NOT NULL default '0' AFTER dataint3;
ALTER TABLE gossip_scripts ADD COLUMN comments varchar(255) NOT NULL AFTER o;

ALTER TABLE quest_end_scripts ADD COLUMN dataint2 int(11) NOT NULL default '0' AFTER dataint;
ALTER TABLE quest_end_scripts ADD COLUMN dataint3 int(11) NOT NULL default '0' AFTER dataint2;
ALTER TABLE quest_end_scripts ADD COLUMN dataint4 int(11) NOT NULL default '0' AFTER dataint3;
ALTER TABLE quest_end_scripts ADD COLUMN comments varchar(255) NOT NULL AFTER o;

ALTER TABLE quest_start_scripts ADD COLUMN dataint2 int(11) NOT NULL default '0' AFTER dataint;
ALTER TABLE quest_start_scripts ADD COLUMN dataint3 int(11) NOT NULL default '0' AFTER dataint2;
ALTER TABLE quest_start_scripts ADD COLUMN dataint4 int(11) NOT NULL default '0' AFTER dataint3;
ALTER TABLE quest_start_scripts ADD COLUMN comments varchar(255) NOT NULL AFTER o;

ALTER TABLE spell_scripts ADD COLUMN dataint2 int(11) NOT NULL default '0' AFTER dataint;
ALTER TABLE spell_scripts ADD COLUMN dataint3 int(11) NOT NULL default '0' AFTER dataint2;
ALTER TABLE spell_scripts ADD COLUMN dataint4 int(11) NOT NULL default '0' AFTER dataint3;
ALTER TABLE spell_scripts ADD COLUMN comments varchar(255) NOT NULL AFTER o;
