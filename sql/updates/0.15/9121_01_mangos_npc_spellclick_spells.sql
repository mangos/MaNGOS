ALTER TABLE db_version CHANGE COLUMN required_9095_01_mangos_command required_9121_01_mangos_npc_spellclick_spells bit;

UPDATE npc_spellclick_spells SET cast_flags=1 WHERE spell_id IN (50926,51026,51592,51961);
DELETE FROM npc_spellclick_spells WHERE spell_id IN (50927,50737,51593,51037);
