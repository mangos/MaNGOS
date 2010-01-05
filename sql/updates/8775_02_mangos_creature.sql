ALTER TABLE db_version CHANGE COLUMN required_8775_01_mangos_creature_template required_8775_02_mangos_creature bit;

-- cause bgs now have different spawnmodes all creatures on those maps must go
-- to all spwanmodes.. maybe this isn't valid for all creatures - but i won't
-- destroy again all bgs :p
-- 0x1 = 2^0 - normal
-- 0x2 = 2^1 - difficulty_1
-- 0x4 = 2^2 - difficulty_2
-- 0x8 = 2^3 - difficulty_3
UPDATE creature SET spawnMask = (0x1 | 0x2 | 0x4 | 0x8) WHERE map IN (30, 489, 529, 566);
