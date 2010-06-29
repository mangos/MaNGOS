ALTER TABLE db_version CHANGE COLUMN required_10109_01_mangos_creature_model_info required_10119_01_mangos_creature_model_info bit;

DELETE FROM creature_model_info WHERE modelid IN (57,58);
INSERT INTO creature_model_info (modelid, bounding_radius, combat_reach, gender, modelid_other_gender) VALUES
(57, 0.3830, 1.5, 0, 58),
(58, 0.3830, 1.5, 1, 57);
