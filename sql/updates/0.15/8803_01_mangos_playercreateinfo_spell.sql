ALTER TABLE db_version CHANGE COLUMN required_8800_01_mangos_spell_elixir required_8803_01_mangos_playercreateinfo_spell bit;

UPDATE `playercreateinfo_spell` SET `spell` = 26297 WHERE `spell` IN (20554,26296,50621);
