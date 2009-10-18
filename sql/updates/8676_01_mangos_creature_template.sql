ALTER TABLE db_version CHANGE COLUMN required_8618_01_mangos_spell_proc_event required_8676_01_mangos_creature_template bit;

-- set all spirithealer and spiritguides to be visible only for dead people
UPDATE creature_template SET flags_extra = flags_extra | 0x200
WHERE npcflag & (16384 | 32768);
