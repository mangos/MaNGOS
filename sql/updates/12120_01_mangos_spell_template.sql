ALTER TABLE db_version CHANGE COLUMN required_12112_14_mangos_command required_12120_01_mangos_spell_template bit;

ALTER TABLE spell_template ADD COLUMN effect0_misc_value_b int(11) unsigned NOT NULL DEFAULT '0' AFTER effect0_misc_value;
