-- Execute on MaNGOS DB

-- Fire bomb script

DELETE FROM `creature_ai_scripts` WHERE `creature_id` = 18225;
INSERT INTO `creature_ai_scripts`(`creature_id`,`event_type`,`event_inverse_phase_mask`,`event_chance`
,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action1_type`,`action1_param1`
,`action1_param2`,`action1_param3`,`action2_type`,`action2_param1`,`action2_param2`,`action2_param3`,
`action3_type`,`action3_param1`,`action3_param2`,`action3_param3`,`comment`) VALUES
(18225,11,0,100,0,0,0,0,0,11,31961,0,4,0,0,0,0,0,0,0,0,'Fire Bomb Target cast Fire Bomb on Spawn'),
(18225,0,100,5000,0,5000,0,0,0,37,0,0,0,0,0,0,0,0,0,0,0,'Fire Bomb Target Despawn');

UPDATE creature_template SET AIName = 'EventAI', ScriptName='' WHERE `entry` = 18225;
