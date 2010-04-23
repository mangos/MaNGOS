ALTER TABLE db_version CHANGE COLUMN required_8549_03_mangos_battleground_events required_8573_01_mangos_mangos_string bit;

DELETE FROM mangos_string WHERE entry IN(573,574);
