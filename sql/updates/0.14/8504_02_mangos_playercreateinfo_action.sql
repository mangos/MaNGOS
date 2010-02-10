ALTER TABLE db_version CHANGE COLUMN required_8504_01_mangos_playercreateinfo_spell required_8504_02_mangos_playercreateinfo_action bit;

UPDATE `playercreateinfo_action`
 SET `action` = 21084
 WHERE `action` = 20154 AND `type` = 0;
