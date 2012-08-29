ALTER TABLE db_version CHANGE COLUMN required_12121_01_mangos_spell_template required_12138_01_mangos_command bit;

DELETE FROM `command` WHERE `name` = 'honor update';

