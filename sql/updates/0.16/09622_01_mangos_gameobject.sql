ALTER TABLE db_version CHANGE COLUMN required_9590_01_mangos_db_script_string required_9622_01_mangos_gameobject bit;

ALTER TABLE gameobject ADD KEY idx_map(map);
ALTER TABLE gameobject ADD KEY idx_id(id);
