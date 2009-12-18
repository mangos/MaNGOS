ALTER TABLE db_version CHANGE COLUMN required_9015_01_mangos_spell_bonus_data required_9018_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 60089;
INSERT INTO spell_bonus_data VALUES
(60089,0,0,0.05,'Druid - Faerie Fire (Feral) Triggered');
