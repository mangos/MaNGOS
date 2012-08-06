ALTER TABLE db_version CHANGE COLUMN required_10263_02_mangos_pool_gameobject required_10263_03_mangos_pool_pool bit;

DROP TABLE IF EXISTS pool_pool_temp;
CREATE TABLE pool_pool_temp
SELECT pool_id, mother_pool, chance, description FROM pool_pool GROUP BY pool_id;

ALTER TABLE pool_pool_temp
  ADD PRIMARY KEY  (pool_id),
  ADD INDEX pool_idx (mother_pool);

DROP TABLE IF EXISTS pool_pool;
RENAME TABLE pool_pool_temp TO pool_pool;
