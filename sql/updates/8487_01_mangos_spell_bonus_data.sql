ALTER TABLE db_version CHANGE COLUMN required_8482_01_mangos_spell_elixir required_8487_01_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` where entry in (20424, 20467, 42463, 53739, 31803, 53742, 31804, 53733, 31893, 32221, 32220, 31898, 53719, 53718, 53725, 53726);
INSERT INTO `spell_bonus_data` VALUES
(20424, 0, 0, 0, "Paladin - Seal of Command Proc"),
(20467, 0.25, 0, 0.16, "Paladin - Judgement of Command"),
(42463, 0, 0.00156, 0.003, "Paladin - Seal of Vengeance (full stack proc)"),
(53739, 0, 0.00156, 0.003, "Paladin - Seal of Corruption (full stack proc)"),
(31803, 0, 0.0156, 0.03, "Paladin - Holy Vengeance"),
(53742, 0, 0.0156, 0.03, "Paladin - Blood Corruption"),
(31804, 0, 0, 0, "Paladin - Judgement of Vengeance"),
(53733, 0, 0, 0, "Paladin - Judgement of Corruption"),
(31893, 0, 0, 0, "Paladin - Seal of Blood Proc Enemy"),
(32221, 0, 0, 0, "Paladin - Seal of Blood Proc Self"),
(31898, 0.18, 0, 0.11, "Paladin - Judgement of Blood Enemy"),
(32220, 0.0594, 0, 0.0363, "Paladin - Judgement of Blood Self"),
(53719, 0, 0, 0, "Paladin - Seal of the Martyr Proc Enemy"),
(53718, 0, 0, 0, "Paladin - Seal of the Martyr Proc Self"),
(53726, 0.18, 0, 0.11, "Paladin - Judgement of the Martyr Enemy"),
(53725, 0.0594, 0, 0.0363, "Paladin - Judgement of the Martyr Self");
