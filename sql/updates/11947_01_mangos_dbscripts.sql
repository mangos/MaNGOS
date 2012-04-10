ALTER TABLE db_version CHANGE COLUMN required_11940_07_mangos_spell_scripts required_11947_01_mangos_dbscripts bit;

-- Update teleport
UPDATE creature_movement_scripts SET data_flags=data_flags|8 WHERE command=3 AND datalong2=0 AND (x!=0 AND y!=0 AND z!=0);
UPDATE event_scripts SET data_flags=data_flags|8 WHERE command=3 AND datalong2=0 AND (x!=0 AND y!=0 AND z!=0);
UPDATE gameobject_scripts SET data_flags=data_flags|8 WHERE command=3 AND datalong2=0 AND (x!=0 AND y!=0 AND z!=0);
UPDATE gossip_scripts SET data_flags=data_flags|8 WHERE command=3 AND datalong2=0 AND (x!=0 AND y!=0 AND z!=0);
UPDATE quest_end_scripts SET data_flags=data_flags|8 WHERE command=3 AND datalong2=0 AND (x!=0 AND y!=0 AND z!=0);
UPDATE quest_start_scripts SET data_flags=data_flags|8 WHERE command=3 AND datalong2=0 AND (x!=0 AND y!=0 AND z!=0);
UPDATE spell_scripts SET data_flags=data_flags|8 WHERE command=3 AND datalong2=0 AND (x!=0 AND y!=0 AND z!=0);

-- Set all move commands to default creature speed
UPDATE creature_movement_scripts SET datalong2=0 WHERE command=3;
UPDATE event_scripts SET datalong2=0 WHERE command=3;
UPDATE gameobject_scripts SET datalong2=0 WHERE command=3;
UPDATE gossip_scripts SET datalong2=0 WHERE command=3;
UPDATE quest_end_scripts SET datalong2=0 WHERE command=3;
UPDATE quest_start_scripts SET datalong2=0 WHERE command=3;
UPDATE spell_scripts SET datalong2=0 WHERE command=3;
