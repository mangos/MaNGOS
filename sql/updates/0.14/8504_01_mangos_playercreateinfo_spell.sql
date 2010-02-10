ALTER TABLE db_version CHANGE COLUMN required_8499_01_mangos_spell_elixir required_8504_01_mangos_playercreateinfo_spell bit;

UPDATE `playercreateinfo_spell`
 SET `spell` = 21084
 WHERE `spell` = 20154;
