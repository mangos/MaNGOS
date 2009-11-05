ALTER TABLE db_version CHANGE COLUMN required_8775_02_mangos_creature required_8775_03_mangos_gameobject bit;

-- cause bgs now have different spawnmodes all gameobjects on those maps must go
-- to all spwanmodes.. maybe this isn't valid for all gameobjects - but i won't
-- destroy again all bgs :p
UPDATE gameobject SET spawnMask = (0x1 | 0x2 | 0x4 | 0x8) WHERE map IN (30, 489, 529, 566);
