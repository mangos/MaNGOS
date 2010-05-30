ALTER TABLE db_version CHANGE COLUMN required_9309_01_mangos_quest_template required_9310_01_mangos_spell_elixir bit;

DELETE FROM `spell_elixir` WHERE `entry`=17624;
