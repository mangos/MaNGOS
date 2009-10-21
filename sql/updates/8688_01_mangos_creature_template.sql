ALTER TABLE db_version CHANGE COLUMN required_8676_01_mangos_creature_template required_8688_01_mangos_creature_template bit;

-- reverts last update - we now have something better
UPDATE creature_template SET flags_extra = flags_extra & ~(0x200) WHERE npcflag
& (16384|32768);
