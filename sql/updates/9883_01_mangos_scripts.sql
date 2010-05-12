ALTER TABLE db_version CHANGE COLUMN required_9881_01_mangos_scripts required_9883_01_mangos_scripts bit;

-- convert to CHAT_TYPE_WHISPER
UPDATE event_scripts SET datalong=4 WHERE command=0 AND datalong=1;
UPDATE gameobject_scripts SET datalong=4 WHERE command=0 AND datalong=1;
UPDATE gossip_scripts SET datalong=4 WHERE command=0 AND datalong=1;
UPDATE quest_end_scripts SET datalong=4 WHERE command=0 AND datalong=1;
UPDATE quest_start_scripts SET datalong=4 WHERE command=0 AND datalong=1;
UPDATE spell_scripts SET datalong=4 WHERE command=0 AND datalong=1;

-- convert to CHAT_TYPE_YELL
UPDATE event_scripts SET datalong=1 WHERE command=0 AND datalong=2;
UPDATE gameobject_scripts SET datalong=1 WHERE command=0 AND datalong=2;
UPDATE gossip_scripts SET datalong=1 WHERE command=0 AND datalong=2;
UPDATE quest_end_scripts SET datalong=1 WHERE command=0 AND datalong=2;
UPDATE quest_start_scripts SET datalong=1 WHERE command=0 AND datalong=2;
UPDATE spell_scripts SET datalong=1 WHERE command=0 AND datalong=2;

-- convert to CHAT_TYPE_TEXT_EMOTE
UPDATE event_scripts SET datalong=2 WHERE command=0 AND datalong=3;
UPDATE gameobject_scripts SET datalong=2 WHERE command=0 AND datalong=3;
UPDATE gossip_scripts SET datalong=2 WHERE command=0 AND datalong=3;
UPDATE quest_end_scripts SET datalong=2 WHERE command=0 AND datalong=3;
UPDATE quest_start_scripts SET datalong=2 WHERE command=0 AND datalong=3;
UPDATE spell_scripts SET datalong=2 WHERE command=0 AND datalong=3;
