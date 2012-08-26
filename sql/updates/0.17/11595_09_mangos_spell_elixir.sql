ALTER TABLE db_version CHANGE COLUMN required_11567_01_mangos_spell_proc_event required_11595_09_mangos_spell_elixir bit;

DELETE FROM spell_elixir WHERE entry='63729';
INSERT INTO spell_elixir VALUES
(63729,0x1);
