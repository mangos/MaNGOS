ALTER TABLE db_version CHANGE COLUMN required_10743_01_mangos_spell_chain required_10743_02_mangos_spell_bonus_data bit;

DELETE FROM `spell_bonus_data` WHERE `entry` IN (2818,42243,13797,13812,1495,19306,3044,42245,3674,9007,1822,33745,48628,703,1978,55095,55078);
INSERT INTO `spell_bonus_data` VALUES
(2818,0,0,0,0.03,'Rogue - Deadly Poison'),
(3674,0,0,0,0.02,'Hunter - Black Arrow'),
(9007,0,0,0,0.03,'Druid - Pounce Bleed'),
(1822,0,0,0,0.06,'Druid - Rake'),
(33745,0,0,0.01,0.01,'Druid - Lacerate'),
(48628,0,0,0,0.15,'Druid - Lock Jaw'),
(703,0,0,0,0.07,'Rogue - Garrote'),
(1495,0,0,0.2,0,'Hunter - Mongoose Bite'),
(42243,0,0,0.0837,0,'Hunter - Volley'),
(1978,0,0,0,0.04,'Hunter - Serpent Sting'),
(3044,0,0,0.15,0,'Hunter - Arcane Shot'),
(13797,0,0,0,0.02,'Hunter - Immolation Trap'),
(13812,0,0,0.1,0.1,'Hunter - Explosive Trap'),
(19306,0,0,0.2,0,'Hunter - Counterattack'),
(55095,0,0,0,0.06325,'Death Knight - Frost Fever'),
(55078,0,0,0,0.06325,'Death Knight - Blood Plague');
