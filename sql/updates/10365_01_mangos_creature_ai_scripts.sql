ALTER TABLE db_version CHANGE COLUMN required_10362_01_mangos_creature_movement_template required_10365_01_mangos_creature_ai_scripts bit;

UPDATE creature_ai_scripts SET action1_type=43, action1_param1=0 WHERE action1_type=17 AND action1_param1=68;
UPDATE creature_ai_scripts SET action2_type=43, action2_param1=0 WHERE action2_type=17 AND action2_param1=68;
UPDATE creature_ai_scripts SET action3_type=43, action3_param1=0 WHERE action3_type=17 AND action3_param1=68;
