ALTER TABLE db_version CHANGE COLUMN required_11453_01_mangos_spell_proc_event required_11489_01_mangos_creature_template bit;

UPDATE creature_template SET flags_extra=flags_extra|0x00000400 WHERE npcflag=npcflag|0x10000000;
UPDATE creature_template SET npcflag=npcflag &~ 0x10000000;
