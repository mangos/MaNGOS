ALTER TABLE db_version CHANGE COLUMN required_8803_01_mangos_playercreateinfo_spell required_8803_02_mangos_playercreateinfo_action bit;

UPDATE `playercreateinfo_action` 
 SET `action` = 26297
 WHERE `action` IN (20554,26296,50621) AND `type` = 0;
