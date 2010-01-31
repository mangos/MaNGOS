ALTER TABLE db_version CHANGE COLUMN required_9262_01_mangos_quest_template required_9277_01_mangos_spell_bonus_data bit;

DELETE FROM spell_bonus_data WHERE entry = 60089;
INSERT INTO spell_bonus_data VALUES
(60089,0,0,0.15,'Druid - Faerie Fire (Feral) Triggered');
