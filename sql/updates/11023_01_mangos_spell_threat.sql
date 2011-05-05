ALTER TABLE db_version CHANGE COLUMN required_11018_01_mangos_command required_11023_01_mangos_spell_threat bit;

ALTER TABLE spell_threat ADD COLUMN multiplier FLOAT NOT NULL DEFAULT 1.0 COMMENT 'threat multiplier for damage/healing' AFTER Threat;

ALTER TABLE spell_threat ADD COLUMN ap_bonus FLOAT NOT NULL DEFAULT 0.0 COMMENT 'additional threat bonus from attack power' AFTER multiplier;
