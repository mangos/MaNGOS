ALTER TABLE db_version CHANGE COLUMN required_8777_01_mangos_creature required_8777_02_mangos_gameobject bit;

UPDATE gameobject SET spawnMask = 0x1 WHERE map IN (489, 529, 566);
UPDATE gameobject SET spawnMask = (0x1 | 0x2 | 0x4) WHERE map IN (30);
