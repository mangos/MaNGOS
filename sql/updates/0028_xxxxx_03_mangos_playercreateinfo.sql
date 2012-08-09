ALTER TABLE db_version CHANGE COLUMN required_0028_xxxxx_02_mangos_quest_phase_maps required_0028_xxxxx_03_mangos_playercreateinfo bit;

ALTER TABLE `playercreateinfo`
ADD COLUMN `phaseMap` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `orientation`;
