ALTER TABLE db_version CHANGE COLUMN required_8775_03_mangos_gameobject required_8777_01_mangos_creature bit;

UPDATE creature SET spawnMask = 0x1 WHERE map IN (489, 529, 566);
UPDATE creature SET spawnMask = (0x1 | 0x2 | 0x4) WHERE map IN (30);
