ALTER TABLE character_db_version CHANGE COLUMN required_8828_01_characters_instance_reset required_8843_01_characters bit;

DELETE FROM `character_spell` WHERE `spell` IN (31892, 53720);
DELETE FROM `character_spell_cooldown` WHERE `spell` IN (31892, 53720);
DELETE FROM `character_aura` WHERE `spell` IN (31892, 53720);
DELETE FROM `character_action` WHERE `action` IN (31892, 53720) AND `type`=0;
